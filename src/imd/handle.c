/**\file imd/handle.c
 * Packet handler for the IMD queues
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#define IPRP_FILE IMD_HANDLE

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include "imd.h"

extern time_t curr_time;
extern list_t active_senders;

/* Function prototypes */
int handle_packet(struct nfq_q_handle *queue, struct nfgenmsg *message, struct nfq_data *packet, void *data);
int ird_handle_packet(struct nfq_q_handle *queue, struct nfgenmsg *message, struct nfq_data *packet, void *data);
int global_handle(struct nfq_q_handle *queue, struct nfq_data *packet, bool iprp_message);
iprp_active_sender_t *activesenders_find_entry(const IPRP_INADDR src_addr, const IPRP_INADDR dest_addr, const uint16_t src_port, const uint16_t dest_port);
iprp_active_sender_t *activesenders_create_entry(const IPRP_INADDR src_addr, const IPRP_INADDR dest_addr, const uint16_t src_port, const uint16_t dest_port, const bool iprp_enabled);

/**
 Sets up and launches the wrapper for the IMD queue

 The IMD queue gets all packets sent to any monitored port from non-iPRP sources.
 Those packets can come either from an host to which no iPRP session has been established,
 or a periodical packet to allow newly joining host to the multicast group to extablish their session.
*/
void* handle_routine(void* arg) {
	uint16_t queue_id = (uint16_t) arg;
	DEBUG("In routine");

	// Setup NFQueue
	iprp_queue_t nfq;
	queue_setup(&nfq, queue_id, handle_packet);
	DEBUG("NFQueue setup (%d)", queue_id);

	// Handle outgoing packets
	while (true) {
		// Get packet
		int err = get_and_handle(nfq.handle, nfq.fd);
		if (err) {
			if (err == IPRP_ERR) {
				ERR("Unable to retrieve packet from IMD queue", errno);
			}
			DEBUG("Error %d while handling packet", err);
		}
		DEBUG("Packet handled");
	}
}
int handle_packet(struct nfq_q_handle *queue, struct nfgenmsg *message, struct nfq_data *packet, void *data) {
	DEBUG("Handling packet");

	return global_handle(queue, packet, false);
}

/**
 Sets up and launches the wrapper for the IRD-IMD queue

 The IRD-IMD queue transfers packets received by the IRD to update the IMD structures needed.
*/
void* ird_handle_routine(void* arg) {
	uint16_t queue_id = (uint16_t) arg;
	DEBUG("IRD In routine");

	// Setup NFQueue
	iprp_queue_t nfq;
	queue_setup(&nfq, queue_id, ird_handle_packet);
	DEBUG("IRD NFQueue setup (%d)", queue_id);

	// Handle outgoing packets
	while (true) {
		// Get packet
		int err = get_and_handle(nfq.handle, nfq.fd);
		if (err) {
			if (err == IPRP_ERR) {
				ERR("Unable to retrieve packet from IMD-IRD queue", errno);
			}
			DEBUG("Error %d while handling packet", err);
		}
		DEBUG("Packet handled");
	}
}
int ird_handle_packet(struct nfq_q_handle *queue, struct nfgenmsg *message, struct nfq_data *packet, void *data) {
	DEBUG("Handling packet");

	return global_handle(queue, packet, true);
}

/**
 Handle the message coming from one of the two queues monitored by the IMD

 The handler first creates or updates the active senders entry for the incoming packet.
 It then accepts the packet if it is an iPRP packet, or if no session is established yet.
 Otherwise it rejects the packet (if the packet is a non-iPRP packet sent from an iPRP host).
*/
int global_handle(struct nfq_q_handle *queue, struct nfq_data *packet, bool iprp_message) {
	// Get packet payload
	int bytes;
	unsigned char *buf;
	if ((bytes = nfq_get_payload(packet, &buf)) == -1) {
		ERR("Unable to retrieve payload from received packet", IPRP_ERR_NFQUEUE);
	}
	DEBUG("Got payload");

	// Get payload headers and source address
	IPRP_IPHDR *ip_header = (IPRP_IPHDR *) buf;
	struct udphdr *udp_header = (struct udphdr *) (buf + sizeof(IPRP_IPHDR));
	DEBUG("Got IP/UDP headers");
	
	// Get link information
	IPRP_INADDR src_addr;
	HEADER_TO_ADDR(src_addr, ip_header->saddr);
	IPRP_INADDR dest_addr;
	HEADER_TO_ADDR(dest_addr, ip_header->daddr);
	uint16_t src_port = ntohs(udp_header->source);
	uint16_t dest_port = ntohs(udp_header->dest);
	DEBUG("Got link information");

	// Protect the whole process from concurrent cleanup (=> better performance)
	list_lock(&active_senders);

	// Find corresponding entry in active senders
	iprp_active_sender_t *entry = activesenders_find_entry(src_addr, dest_addr, src_port, dest_port);
	DEBUG("Finished searching for senders");

	// Create entry if not present
	if (!entry) {
		// Create entry
		entry = activesenders_create_entry(src_addr, dest_addr, src_port, dest_port, iprp_message);
		if (!entry) {
			list_unlock(&active_senders);
			ERR("malloc failed to create active senders entry", errno);
		}
		DEBUG("Active senders entry created");
		
		// Add entry to active senders
		list_append(&active_senders, entry);
		DEBUG("Active senders entry stored");
	}
	
	// Update entry timer
	entry->last_seen = curr_time;
#ifdef IPRP_MULTICAST
	if (iprp_message) {
		entry->iprp_enabled = true;
	}
#endif
	DEBUG("Entry timer updated");

	// Allow maintenance work to continue
	list_unlock(&active_senders);

	// Accept of reject packet accordingly
	struct nfqnl_msg_packet_hdr *nfq_header = nfq_get_msg_packet_hdr(packet);
	if (!nfq_header) {
		ERR("Unable to retrieve header from received packet", IPRP_ERR_NFQUEUE);
	}
	DEBUG("Got header");
#ifndef IPRP_MULTICAST
	uint32_t verdict = NF_ACCEPT;
#else
	uint32_t verdict = (iprp_message || !entry->iprp_enabled) ? NF_ACCEPT : NF_DROP;
#endif
	if (nfq_set_verdict(queue, ntohl(nfq_header->packet_id), verdict, bytes, buf) == -1) {
		ERR("Unable to set verdict", IPRP_ERR_NFQUEUE);
	}
	LOG((verdict == NF_ACCEPT) ? "Packet accepted" : "Packet dropped");

	return 0;
}

/**
 Find an entry in the active senders cache corresponding to the given parameters
*/
iprp_active_sender_t *activesenders_find_entry(const IPRP_INADDR src_addr, const IPRP_INADDR dest_addr, const uint16_t src_port, const uint16_t dest_port) {
	// Find the entry
	list_elem_t *iterator = active_senders.head;
	while(iterator != NULL) {
		iprp_active_sender_t *as = (iprp_active_sender_t*) iterator->elem;
		
		if (COMPARE_ADDR(src_addr, as->src_addr)
			&& COMPARE_ADDR(dest_addr, as->dest_addr)
			&& src_port == as->src_port
			&& dest_port == as->dest_port) {
			return as;
		}
		iterator = iterator->next;
	}
	return NULL;
}

/**
 Creates an active sender entry with the corresponding parameters
*/
iprp_active_sender_t *activesenders_create_entry(const IPRP_INADDR src_addr, const IPRP_INADDR dest_addr, const uint16_t src_port, const uint16_t dest_port, const bool iprp_enabled) {
	iprp_active_sender_t *entry = malloc(sizeof(iprp_active_sender_t));
	if (!entry) {
		return NULL;
	}

	entry->src_addr = src_addr;
	entry->dest_addr = dest_addr;
	entry->src_port = src_port;
	entry->dest_port = dest_port;
#ifdef IPRP_MULTICAST
	entry->iprp_enabled = iprp_enabled;
#endif

	return entry;
}
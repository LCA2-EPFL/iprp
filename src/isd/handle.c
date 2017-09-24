/**\file isd/handle.c
 * Packet handler for the ISD queue
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#define IPRP_FILE ISD_HANDLE

#include <errno.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include "isd.h"
#include "peerbase.h"

extern time_t curr_time;
#ifdef IPRP_MULTICAST
time_t last_allowed_thru = 0; /* Multicast periodical keepalive timer */
#endif

extern iprp_isd_peerbase_t pb;
extern int sockets[IPRP_MAX_INDS];

/* Function prototypes */
int handle_packet(struct nfq_q_handle *queue, struct nfgenmsg *message, struct nfq_data *packet, void *data);
size_t create_iprp_packet(struct nfq_data *packet, char* *new_buf, struct nfq_q_handle *queue);
int send_packet(iprp_ind_t ind, char *packet, size_t packet_size, IPRP_SOCKADDR *addr, iprp_ind_bitmap_t base_inds);
uint32_t get_verdict();

/**
 Sets up the queue and forwards packet to the handle function
*/
void* handle_routine(void* arg) {
	intptr_t queue_id = (intptr_t) arg;
	DEBUG("In routine");

	// Setup NFQueue
	iprp_queue_t nfq;
	queue_setup(&nfq, queue_id, handle_packet);
	DEBUG("NFQueue setup");

	// Wait for the peerbase to be loaded the first time
	pthread_mutex_lock(&pb.mutex);
	while (!pb.loaded) {
		pthread_cond_wait(&pb.cond, &pb.mutex);
	}
	pthread_mutex_unlock(&pb.mutex);
	DEBUG("Base loaded");

	// Handle outgoing packets
	while (true) {
		// Get packet
		int err = get_and_handle(nfq.handle, nfq.fd);
		if (err) {
			if (err == IPRP_ERR) {
				ERR("Unable to retrieve packet from ISD queue", errno);
			}
			DEBUG("Error %d while handling packet", err);
		}
		DEBUG("Packet handled");
	}
}

/**
 Duplicates a packet and sends it through iPRP

 The routine first adds the iPRP header to the packet.
 It then sends it to all the receiver interfaces contained in the peerbase.
*/
int handle_packet(struct nfq_q_handle *queue, struct nfgenmsg *message, struct nfq_data *packet, void *data) {
	DEBUG("In handle");

	// Create new packet
	char* new_packet = NULL;
	size_t new_size = create_iprp_packet(packet, &new_packet, queue);
	if (new_size == 0) {
		ERR("Unable to create IPRP packet", errno);
	}
	DEBUG("New packet of size %lu created", new_size);

	IPRP_SOCKADDR dest_addr;
#ifndef IPRP_MULTICAST
	// Send packet on all interfaces
	pthread_mutex_lock(&pb.mutex);

	for (int i = 0; i < IPRP_MAX_INDS; ++i) {
		sockaddr_fill(&dest_addr, pb.base.dest_addr[i], IPRP_DATA_PORT);

		int err = send_packet(i, new_packet, new_size, &dest_addr, pb.base.inds);
		if (err) {
			pthread_mutex_unlock(&pb.mutex);
			ERR("Unable to send packet", errno);			
		}
	}

	pthread_mutex_unlock(&pb.mutex);
#else
	// Send packet to group
	sockaddr_fill(&dest_addr, pb.base.link.dest_addr, IPRP_DATA_PORT);

	pthread_mutex_lock(&pb.mutex);
	
	for (int i = 0; i < IPRP_MAX_INDS; ++i) {
		int err = send_packet(i, new_packet, new_size, &dest_addr, pb.base.inds);
		if (err) {
			pthread_mutex_unlock(&pb.mutex);
			ERR("Unable to send packet", errno);			
		}
	}
	
	pthread_mutex_unlock(&pb.mutex);
#endif
	
	DEBUG("Outgoing packet handled. All duplicate packets sent.");
	
	return 0;
}

/**
 Creates an iPRP packet from the given NFQueue packet
*/
size_t create_iprp_packet(struct nfq_data *packet, char* *new_buf, struct nfq_q_handle *queue) {
	static uint32_t seq_nb = 1;
	int bytes;
	unsigned char *buf;
	
	// Get packet payload
	if ((bytes = nfq_get_payload(packet, &buf)) == -1) {
		ERR("Unable to retrieve payload from received packet", IPRP_ERR_NFQUEUE);
	}
	DEBUG("Got payload");

	// Create new packet
	char *payload = buf + sizeof(IPRP_IPHDR) + sizeof(struct udphdr);
	size_t payload_size = bytes - sizeof(IPRP_IPHDR) - sizeof(struct udphdr);
	char* new_packet = malloc(payload_size + sizeof(iprp_header_t));
	if (!new_packet) {
		return 0;
	}
	memcpy(new_packet + sizeof(iprp_header_t), payload, payload_size);
	DEBUG("Created new packet");

	// Create IPRP header
	iprp_header_t *header = (iprp_header_t *) new_packet;
	header->version = IPRP_VERSION;
	header->seq_nb = seq_nb;
	header->dest_port = pb.base.link.dest_port;
#ifndef IPRP_MULTICAST
	header->dest_addr = pb.base.link.dest_addr;
#endif
	memcpy(&header->snsid, pb.base.link.snsid, IPRP_SNSID_SIZE);
	DEBUG("IPRP header created");

	// Compute next sequence number
	seq_nb = (seq_nb == UINT32_MAX) ? 1 : seq_nb + 1;

	// Apply changes to the given pointer
	*new_buf = new_packet;

	// Set verdict in queue
#ifndef IPRP_MULTICAST
	uint32_t verdict = NF_DROP;
#else
	uint32_t verdict = get_verdict();
#endif
	struct nfqnl_msg_packet_hdr *nfq_header = nfq_get_msg_packet_hdr(packet);
	if (!nfq_header) {
		ERR("Unable to retrieve header form received packet", IPRP_ERR_NFQUEUE);
	}
	if (nfq_set_verdict(queue, ntohl(nfq_header->packet_id), verdict, bytes, buf) == -1) {
		ERR("Unable to set verdict", IPRP_ERR_NFQUEUE);
	}
	DEBUG("Packet verdict set to %u", verdict);

	return payload_size + sizeof(iprp_header_t);
}

/**
 Sends a packet on the given interface
*/
int send_packet(iprp_ind_t ind, char *packet, size_t packet_size, IPRP_SOCKADDR *addr, iprp_ind_bitmap_t base_inds) {
	if ((1 << ind) & base_inds) {
		((iprp_header_t *) packet)->ind = ind;
		if (sendto(sockets[ind], packet, packet_size, 0, (struct sockaddr *) addr, sizeof(IPRP_SOCKADDR)) == -1) {
			return IPRP_ERR;
		}
		DEBUG("Packet sent on IND %d", ind);
	}
	return 0;
}

#ifdef IPRP_MULTICAST
/**
 Returns whether the packet should be let through (multicast only)
*/
uint32_t get_verdict() {
	if (curr_time - last_allowed_thru >= ISD_T_ALLOW) {
		last_allowed_thru = curr_time;
		return NF_ACCEPT;
	}
	return NF_DROP;
}
#endif
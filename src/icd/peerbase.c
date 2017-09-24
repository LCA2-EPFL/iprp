/**\file icd/peerbase.c
 * Peerbase handler for the ICD side
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#define IPRP_FILE ICD_PB

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "icd.h"
#include "peerbase.h"

extern time_t curr_time;
extern iprp_host_t this;

/* Peerbase cache */
list_t peerbases;

/* Function prototypes */
void create_peerbase(iprp_peerbase_t* peerbase, iprp_icd_base_t *base);
pid_t isd_startup(iprp_icd_base_t *base);

/**
 Pushes changes to known peerbases to the ISDs

 The peerbase routine is executed periodically.
 It first deletes aged, inactive senders.
 It then stores the other peerbases to the disk, allowing the corresponding ISDs to update.
 If necessary, it starts the corresponding ISDs up.
*/
void *pb_routine(void *arg) {
	list_init(&peerbases);
	DEBUG("Peerbases initialized");

	while(true) {
		list_elem_t *iterator = peerbases.head;
		while(iterator) {
			// Fine grain lock, as always
			list_lock(&peerbases);

			iprp_icd_base_t *base = (iprp_icd_base_t *) iterator->elem;

			// Delete aged entry
			if (curr_time - base->last_cap > ICD_T_PB_EXP) {
				list_elem_t *to_delete = iterator;
				iterator = iterator->next;
				free(to_delete->elem);
				list_delete(&peerbases, to_delete);
				DEBUG("Aged entry");
				continue;
			}
			iterator = iterator->next;
			DEBUG("Fresh entry");

			// List stuff over, the rest doesn't need locking
			list_unlock(&peerbases);

			// Fresh entry, create peerbase
			iprp_peerbase_t peerbase;
			create_peerbase(&peerbase, base);
			DEBUG("Peerbase created");

			// Fresh entry, we store it on disk
			char snsid_str[2 * IPRP_SNSID_SIZE + 1];
			for (int i = 0; i < IPRP_SNSID_SIZE; ++i) {
				snprintf(&snsid_str[2*i], 3, "%02x", base->link.snsid[i]);
			}
			char path[IPRP_PEERBASE_PATH_LENGTH];
			snprintf(path, IPRP_PEERBASE_PATH_LENGTH, "files/base_%s.iprp", snsid_str);
			int err;
			if ((err = peerbase_store(path, &peerbase))) {
				ERR("Unable to store peerbase", err);
			}
			DEBUG("Peerbase stored");

			// Launch ISD
			if (base->isd_pid == -1) {
				if ((base->isd_pid = isd_startup(base)) == -1) {
					ERR("Unable to start ISD", errno);
				}
				DEBUG("ISD started")
			}
		}

		sleep(ICD_T_PB_CACHE);
	}
}

/**
 Fills a peerbase structure with an ICD-specific peerbase
*/
void create_peerbase(iprp_peerbase_t* peerbase, iprp_icd_base_t *base) {
	peerbase->link = base->link;
	peerbase->host = this;
	peerbase->inds = base->inds;
#ifndef IPRP_MULTICAST
	for (int i = 0; i < IPRP_MAX_INDS; ++i) {
		peerbase->dest_addr[i] = base->dest_addr[i];
	}
#endif
}

/**
 Start an ISD for the given base
*/
pid_t isd_startup(iprp_icd_base_t *base) {
	pid_t pid = fork();
	if (!pid) { // Child side
		// Create NFqueue
		char dest_addr[IPRP_ADDRSTRLEN];
		inet_ntop(IPRP_AF, &base->link.dest_addr, dest_addr, IPRP_ADDRSTRLEN);
		char shell[120];
	#ifndef IPRP_IPV6
		snprintf(shell, 120, "iptables -t mangle -A POSTROUTING -p udp -d %s --dport %d --sport %d -j NFQUEUE --queue-num %d", dest_addr, base->link.dest_port, base->link.src_port, base->queue_id);
	#else
		snprintf(shell, 120, "ip6tables -t mangle -A POSTROUTING -p udp -d %s --dport %d --sport %d -j NFQUEUE --queue-num %d", dest_addr, base->link.dest_port, base->link.src_port, base->queue_id);
	#endif
		if (system(shell) == -1) {
			ERR("Unable to create nfqueue for ISD", errno);
		}
		DEBUG("NFQueue created for ISD");

		// Launch sender
		char queue_id[16];
		sprintf(queue_id, "%d", base->queue_id);
		char snsid_str[2 * IPRP_SNSID_SIZE + 1];
		for (int i = 0; i < IPRP_SNSID_SIZE; ++i) {
			snprintf(&snsid_str[2*i], 3, "%02x", base->link.snsid[i]);
		}
		char base_path[IPRP_PEERBASE_PATH_LENGTH];
		snprintf(base_path, IPRP_PEERBASE_PATH_LENGTH, "files/base_%s.iprp", snsid_str);
		if (execl(IPRP_ISD_BINARY_LOC, "isd", queue_id, base_path, NULL) == -1) {
			ERR("Unable to launch sender deamon", errno);
		}
	} else {
		return pid;
	}
}
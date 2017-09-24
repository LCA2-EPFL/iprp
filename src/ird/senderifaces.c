/**\file ird/senderifaces.c
 * Sender interfaces handler for the IRD side
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#ifdef IPRP_MULTICAST

#define IPRP_FILE IRD_SI

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "ird.h"
#include "senderifaces.h"

/* Global variables */
list_t sender_ifaces;
int subscribe_socket;

/* Function prototypes */
iprp_sender_ifaces_t *find_si_in_array(iprp_sender_ifaces_t *si, iprp_sender_ifaces_t *array, int count);
bool find_si_in_list(iprp_sender_ifaces_t *si);
bool find_addr_in_sender(IPRP_INADDR addr, iprp_sender_ifaces_t *si);
void add_memberships(iprp_sender_ifaces_t *si);
void update_memberships(iprp_sender_ifaces_t *curr, iprp_sender_ifaces_t *new);
void drop_memberships(iprp_sender_ifaces_t *si);
void membership(IPRP_INADDR group_addr, IPRP_INADDR src_addr, IPRP_INADDR host_addr, bool add);

/**
 Updates the source memberships of the IRD according to changes pushed down by the ICD

 The routine first sets up the socket it needs to subscribe to SSM groups.
 It then periodically reads the sender interfaces file.
 Given the new data, it updates its cache and adds or drops SSM memberships as needed.
*/
void* si_routine(void* arg) {
	DEBUG("In routine");

	// Initialize list
	list_init(&sender_ifaces);
	DEBUG("Initialized sender interfaces list");

	// Create socket for subscription
	if ((subscribe_socket = socket(IPRP_AF, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		ERR("Unable to create socket", errno);
	}
	DEBUG("Socket created");

	while(true) {
		// Read file from ICD
		iprp_sender_ifaces_t *new_sender_ifaces;
		int new_count;
		int err;
		if ((err = senderifaces_load(IPRP_SI_FILE, &new_count, &new_sender_ifaces))) {
			ERR("Unable to retrieve sender interfaces", err);
		}
		DEBUG("Sender interfaces file read");

		// Check against own records and add or drop memberships as needed
		// Drop phase
		list_elem_t *iterator = sender_ifaces.head;
		while (iterator != NULL) {
			iprp_sender_ifaces_t *sender = (iprp_sender_ifaces_t *) iterator->elem;
			iprp_sender_ifaces_t *new_sender = find_si_in_array(sender, new_sender_ifaces, new_count);
			if (!new_sender) {
				// Delete entry from the list
				drop_memberships(sender);
				list_elem_t *to_delete = iterator;
				iterator = iterator->next;
				list_delete(&sender_ifaces, to_delete);
				DEBUG("Expired sender");
			} else {
				// Update memberships as needed
				update_memberships(sender, new_sender);
				iterator = iterator->next;
				DEBUG("Valid sender");
			}
		}
		DEBUG("End of drop phase");

		// Add phase
		for (int i = 0; i < new_count; ++i) {
			if (!find_si_in_list(&new_sender_ifaces[i])) {
				// Add entry to the list
				iprp_sender_ifaces_t *new_sender = malloc(sizeof(iprp_sender_ifaces_t));
				memcpy(new_sender, &new_sender_ifaces[i], sizeof(iprp_sender_ifaces_t));
				list_append(&sender_ifaces, new_sender);

				// Add all memberships
				add_memberships(new_sender);
				DEBUG("New entry added");
			}
		}
		DEBUG("End of add phase");

		LOG("Subscriptions handled");

		// Sleep
		sleep(IRD_T_SI_CACHE);
	}
}

/**
 Looks for an entry in an array of sender interfaces
*/
iprp_sender_ifaces_t *find_si_in_array(iprp_sender_ifaces_t *si, iprp_sender_ifaces_t *array, int count) {
	for (int i = 0; i < count; ++i) {
		if (COMPARE_ADDR(si->sender_addr, array[i].sender_addr)
				&& COMPARE_ADDR(si->group_addr, array[i].group_addr)) {
			// We have found the entry
			return &array[i];
		}
	}
	return NULL;
}

/**
 Looks for an entry in the list of sender interfaces
*/
bool find_si_in_list(iprp_sender_ifaces_t *si) {
	list_elem_t *iterator = sender_ifaces.head;
	while(iterator != NULL) {
		iprp_sender_ifaces_t *sender = (iprp_sender_ifaces_t *) iterator->elem;

		if (COMPARE_ADDR(sender->sender_addr, si->sender_addr)
				&& COMPARE_ADDR(sender->group_addr, si->group_addr)) {
			// We have found the entry
			return true;
		}

		iterator = iterator->next;
	}
	return false;
}

/**
 Returns whether the given entry contains the given source address
*/
bool find_addr_in_sender(IPRP_INADDR addr, iprp_sender_ifaces_t *si) {
	for (int i = 0; i < si->nb_ifaces; ++i) {
		if (COMPARE_ADDR(si->ifaces[i].addr, addr)) {
			return true;
		}
	}
	return false;
}

/**
 Adds SSM memberships for the given entry
*/
void add_memberships(iprp_sender_ifaces_t *si) {
	for (int i = 0; i < si->nb_ifaces; ++i) {
		membership(si->group_addr, si->ifaces[i].addr, si->host_addr[i], true);
	}
}

/**
 Updates SSM memberships for the old entry given the updated entry
*/
void update_memberships(iprp_sender_ifaces_t *curr, iprp_sender_ifaces_t *new) {
	// Drop phase
	for (int i = 0; i < curr->nb_ifaces; ++i) {
		if (!find_addr_in_sender(curr->ifaces[i].addr, new)) {
			membership(curr->group_addr, curr->ifaces[i].addr, curr->host_addr[i], false);
		}
	}

	// Add phase
	for (int i = 0; i < new->nb_ifaces; ++i) {
		if (!find_addr_in_sender(new->ifaces[i].addr, curr)) {
			membership(new->group_addr, new->ifaces[i].addr, new->host_addr[i], true);
		}
	}
}

/**
 Drops all SSM memberships for the given entry
*/
void drop_memberships(iprp_sender_ifaces_t *si) {
	for (int i = 0; i < si->nb_ifaces; ++i) {
		membership(si->group_addr, si->ifaces[i].addr, si->host_addr[i], false);
	}
}

/**
 Actually adds or drops the SSM membership TODO ipv6
*/
void membership(IPRP_INADDR group_addr, IPRP_INADDR src_addr, IPRP_INADDR host_addr, bool add) {
	struct ip_mreq_source ssm_request;
	ssm_request.imr_multiaddr = group_addr;
	ssm_request.imr_interface = host_addr;
	ssm_request.imr_sourceaddr = src_addr;

	int opt = add ? IP_ADD_SOURCE_MEMBERSHIP : IP_DROP_SOURCE_MEMBERSHIP;
	if (setsockopt(subscribe_socket, IPPROTO_IP, opt, &ssm_request, sizeof(ssm_request)) == -1) {
		ERR("Unable to subscribe to SSM group", errno);
	}
	if (add) {
		DEBUG("Added membership to %x from %x\n", group_addr.s_addr, src_addr.s_addr);
	} else {
		DEBUG("Dropped membership to %x from %x\n", group_addr.s_addr, src_addr.s_addr);		
	}
}

#endif /* IPRP_MULTICAST */
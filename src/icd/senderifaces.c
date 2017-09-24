/**\file icd/senderifaces.c
 * Sender interfaces handler for the ICD side
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#ifdef IPRP_MULTICAST

#define IPRP_FILE ICD_SI

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "icd.h"
#include "senderifaces.h"

extern time_t curr_time;

/* Sender interfaces cache */
list_t sender_ifaces;

/* Function prototypes */
int count_and_cleanup();
iprp_sender_ifaces_t *get_file_contents();

/**
 Pushes the sender interfaces information to the IRD

 The sender interfaces routine periodically deletes the aged entries in its cache (modified by the control routine).
 It then pushes the changes to the disk, where it can be retrieved by the IRD.
*/
void *si_routine(void *arg) {
	DEBUG("In routine");

	list_init(&sender_ifaces);
	senderifaces_store(IPRP_SI_FILE, 0, NULL);
	DEBUG("Sender interfaces initialized");

	while(true) {
		// Delete aged entries
		int count = count_and_cleanup();
		DEBUG("Deleted aged entries");

		// Create active senders file
		iprp_sender_ifaces_t *entries = get_file_contents(count);
		DEBUG("Created sender interfaces file contents");

		// Update on file
		senderifaces_store(IPRP_SI_FILE, count, entries);
		LOG("Active senders file updated");

		sleep(ICD_T_SI_CACHE);
	}
}

/**
 Deletes aged entries and returns the number of valid entries in the list
*/
int count_and_cleanup() {
	int count = 0;
	list_elem_t *iterator = sender_ifaces.head;
	while(iterator != NULL) {
		// Fine grain lock to avoid blocking packet treatment for a long time
		list_lock(&sender_ifaces);

		iprp_sender_ifaces_t *si = (iprp_sender_ifaces_t*) iterator->elem;
		if (curr_time - si->last_seen > ICD_T_SI_EXP) {
			// Expired sender
			list_elem_t *to_delete = iterator;
			iterator = iterator->next;
			free(to_delete->elem);
			list_delete(&sender_ifaces, to_delete);
			DEBUG("Aged entry");
		} else {
			// Good sender
			count++;
			iterator = iterator->next;
			DEBUG("Fresh entry");
		}
		list_unlock(&sender_ifaces);
	}
	return count;
}

/**
 Creates the data to be pushed to the disk
*/
iprp_sender_ifaces_t *get_file_contents(int count) {
	iprp_sender_ifaces_t *entries = calloc(count, sizeof(iprp_sender_ifaces_t));
	if (!entries) {
		ERR("Unable to allocate file contents", errno);
	}

	list_elem_t *iterator = sender_ifaces.head;
	for (int i = 0; i < count; ++i) {
		entries[i] = *((iprp_sender_ifaces_t*) iterator->elem);
		iterator = iterator->next;
	}

	return entries;
}

#endif /* IPRP_MULTICAST */
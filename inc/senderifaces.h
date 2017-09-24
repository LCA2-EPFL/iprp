/**\file senderifaces.h
 * Header file for sender interfaces-related stuff
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */

#ifndef __IPRP_SENDERIFACES_
#define __IPRP_SENDERIFACES_

#include <time.h>
#include <netinet/in.h>

#include "global.h"

#define IPRP_SI_FILE "files/senderifaces.iprp"

/* Sender interfaces entry structure */
typedef struct {
	// Link information
	IPRP_INADDR sender_addr;
	IPRP_INADDR group_addr;

	// Paths information
	int nb_ifaces;
	iprp_iface_t ifaces[IPRP_MAX_IFACE];
	IPRP_INADDR host_addr[IPRP_MAX_IFACE];

	// Status values
	time_t last_seen;
} iprp_sender_ifaces_t;

/* Disk functions */
void senderifaces_store(const char* path, const int count, const iprp_sender_ifaces_t* senders);
int senderifaces_load(const char *path, int* count, iprp_sender_ifaces_t** senders);

#endif /* __IPRP_SENDERIFACES_ */
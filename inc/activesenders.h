/**\file activesenders.h
 * Header file for active senders file-related stuff
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */

#ifndef __IPRP_ACTIVESENDERS_
#define __IPRP_ACTIVESENDERS_

#include <stdbool.h>
#include <stdint.h>

#include "global.h"

#define IPRP_AS_FILE "files/activesenders.iprp"

/**
 The active sender file is the communication medium between the IMD and ICD.
 The IMD fills the file according to the packets it receives (from the IRD or the internet directly).
 The ICD retrieves the information contained in the file and uses it to send CAP messages to the relevant senders.
*/

/* Entry structure */
typedef struct {
	// Link information
	IPRP_INADDR src_addr;
	IPRP_INADDR dest_addr;
	uint16_t src_port;
	uint16_t dest_port;

	// Status values
	time_t last_seen;
#ifdef IPRP_MULTICAST
	bool iprp_enabled;
#endif
} iprp_active_sender_t;

/* Disk functions */
void activesenders_store(const char* path, const int count, const iprp_active_sender_t* senders);
int activesenders_load(const char *path, int* count, iprp_active_sender_t** senders);

#endif /* __IPRP_ACTIVESENDERS_ */
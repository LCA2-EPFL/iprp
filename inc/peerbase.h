/**\file peerbase.h
 * Header file for peerbase-related stuff
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */

#ifndef __IPRP_SENDER_
#define __IPRP_SENDER_

#include "global.h"

#define IPRP_PEERBASE_PATH_LENGTH 57

/**
 The peerbase is the medium of communication between the ICD and the ISDs.
 Each instance of the ISD has its own peerbase.
 The ICD writes the information it gets from the CAP messages it receives into the peerbase.
 The ISD reads this data and configures its outgoing interfaces accordingly.
*/

/* Peerbase structure */
typedef struct {
	iprp_link_t link;
	iprp_host_t host;
	iprp_ind_bitmap_t inds;
#ifndef IPRP_MULTICAST
	IPRP_INADDR dest_addr[IPRP_MAX_INDS];
#endif
} iprp_peerbase_t;

/* Disk functions */
int peerbase_store(const char* path, iprp_peerbase_t *base);
int peerbase_load(const char* path, iprp_peerbase_t *base);

#endif /* __IPRP_SENDER_ */
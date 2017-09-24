/**\file isd.h
 * Header file for ISD
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */

#ifndef __IPRP_ISD_
#define __IPRP_ISD_

#include <stdbool.h>
#include <pthread.h>

#include "global.h"
#include "peerbase.h"

/* ISD structure */
typedef struct {
	iprp_peerbase_t base;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool loaded;
} iprp_isd_peerbase_t;

/* Threads routines */
void* pb_routine(void* arg);
void* handle_routine(void *arg);

#endif /* __IPRP_ISD_ */
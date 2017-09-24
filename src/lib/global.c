/**\file global.c
 * Utility functions
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "global.h"

/**
 Fills a sockaddr_in structure with the given information
*/
void sockaddr_fill(IPRP_SOCKADDR *sockaddr, IPRP_INADDR addr, uint16_t port) {
#ifndef IPRP_IPV6
	sockaddr->sin_family = IPRP_AF;
	sockaddr->sin_port = htons(port);
	sockaddr->sin_addr = addr;
	memset(sockaddr->sin_zero, 0, sizeof(sockaddr->sin_zero));
#else
	sockaddr->sin6_family = IPRP_AF;
	sockaddr->sin6_port = htons(port);
	sockaddr->sin6_addr = addr;
	sockaddr->sin6_flowinfo = 0;
	sockaddr->sin6_scope_id = 0;
#endif
}

/**
 Returns the matching INDs between the given host and INDs
*/
iprp_ind_bitmap_t ind_match(iprp_host_t *sender, iprp_ind_bitmap_t receiver_inds) {
	return sender->inds & receiver_inds;
}

/**
 Returns the current thread name (used for debugging)
*/
char* iprp_thr_name(iprp_thread_t thread) {
	switch(thread) {
		case ICD_MAIN: return "icd";
		case ICD_CTL: return "icd-control";
		case ICD_PORTS: return "icd-ports";
		case ICD_AS: return "icd-as";
		case ICD_PB: return "icd-pb";
	#ifdef IPRP_MULTICAST
		case ICD_SI: return "icd-si";
	#endif

		case ISD_MAIN: return "isd";
		case ISD_HANDLE: return "isd-handle";
		case ISD_PB: return "isd-pb";

		case IMD_MAIN: return "imd";
		case IMD_HANDLE: return "imd-handle";
		case IMD_AS: return "imd-as";

		case IRD_MAIN: return "ird";
		case IRD_HANDLE: return "ird-handle";
	#ifdef IPRP_MULTICAST
		case IRD_SI: return "ird-si";
	#endif
		
		default: return "???";
	}	
}
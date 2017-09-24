/**\file ird.h
 * Header file for IRD
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */

#ifndef __IPRP_IRD_
#define __IPRP_IRD_

#include <stdbool.h>
#include <stdint.h>

#include "global.h"

#define IPRP_DD_MAX_LOST_PACKETS 1024
#define IPRP_STATS_PATH_LENGTH 58

/* Thread routines */
void* handle_routine(void* arg);
void* si_routine(void* arg);

/* Receiver statistics structure */
typedef struct {
	int pkts[IPRP_MAX_INDS];
	time_t last_seen[IPRP_MAX_INDS];
	int wrong[IPRP_MAX_INDS];
	int acc[IPRP_MAX_INDS];
} iprp_receiver_stats_t;

/* Receiver link structure */
typedef struct {
	// Info (fixed) vars
	IPRP_INADDR src_addr;
	uint16_t src_port;
	unsigned char snsid[20];
	// State (variable) vars
	uint32_t list_sn[IPRP_DD_MAX_LOST_PACKETS];
	uint32_t high_sn;
	time_t last_seen;
	// Statistics
	iprp_receiver_stats_t stats;
} iprp_receiver_link_t;

#ifdef IPRP_MULTICAST
 /* SSM-specific structures */
 #ifndef MCAST_JOIN_SOURCE_GROUP
  #define MCAST_JOIN_SOURCE_GROUP 46
 #endif
 
 struct ip_mreqn {
 	struct in_addr imr_multiaddr;
 	struct in_addr imr_address;
 	int imr_ifindex;
 };
 
 struct ip_mreq_source {
 	struct in_addr imr_multiaddr;
 	struct in_addr imr_interface;
 	struct in_addr imr_sourceaddr;
 };

 // TODO ipv6
#endif

/* Statistics functions */
int stats_store(const char* path, iprp_receiver_link_t *stats);
int stats_load(const char* path, iprp_receiver_link_t *stats);

#endif /* __IPRP_IRD_ */
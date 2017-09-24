/**\file debug.h
 * Debugging definitions
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */

#ifndef __IPRP_DEBUG_
#define __IPRP_DEBUG_

#include <stdlib.h>
#include <stdio.h>

// Errors
enum {
	IPRP_ERR = 1,
	IPRP_ERR_MALLOC,
	IPRP_ERR_NOINIT,
	IPRP_ERR_UNKNOWN,
	IPRP_ERR_EMPTY,
	IPRP_ERR_NULLPTR,
	IPRP_ERR_BADFORMAT,
	IPRP_ERR_LOOKUPFAIL,
	IPPR_ERR_MULTIPLE_SAME_IND,
	IPRP_ERR_NFQUEUE
};

#define ERR(msg, var)	printf("Error: %s (%d)\n", msg, var); exit(EXIT_FAILURE)

// Threads
typedef enum {
	ICD_MAIN = (1 << 0),
	ICD_CTL = (1 << 1),
	ICD_PORTS = (1 << 2),
	ICD_PB = (1 << 3),
	ICD_AS = (1 << 4),
#ifdef IPRP_MULTICAST
	ICD_SI = (1 << 5),
#endif

	ISD_MAIN = (1 << 8),
	ISD_PB = (1 << 9),
	ISD_HANDLE = (1 << 10),

	IMD_MAIN = (1 << 16),
	IMD_AS = (1 << 17),
	IMD_HANDLE = (1 << 18),

	IRD_MAIN = (1 << 24),
#ifdef IPRP_MULTICAST
	IRD_SI = (1 << 25),
#endif
	IRD_HANDLE = (1 << 26),
} iprp_thread_t;

char* iprp_thr_name(iprp_thread_t thread);

// Debugging
#define DEBUG_FULL

#define MSG(...) \
	printf("[%s] ", iprp_thr_name(IPRP_FILE)); \
	printf(__VA_ARGS__); \
	printf("\n");

#ifdef DEBUG_NONE
	#define LOG(...) {}
	#define DEBUG(...) {}
#endif

#ifdef DEBUG_INFO
	#define LOG(...) MSG(__VA_ARGS__)
	#define DEBUG(...) {}
#endif

#ifdef DEBUG_FULL
	// Threads needing debugging (flags)
	#define DEBUG_THREADS (-1)

	#define LOG(...) MSG(__VA_ARGS__)

	#define DEBUG(...) \
		if (IPRP_FILE & DEBUG_THREADS) { MSG(__VA_ARGS__) } else {}
#endif

#endif /* __IPRP_DEBUG_ */
/**\file ird.c
 * iPRP Receiving Daemon
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#define IPRP_FILE IRD_MAIN

#include <stdlib.h>
#include <pthread.h>

#include "ird.h"

/* Forwarding queue number */
int imd_queue_id;

/* Threads */
pthread_t time_thread;
pthread_t handle_thread;
#ifdef IPRP_MULTICAST
pthread_t si_thread;
#endif

/**
 Receiver daemon entry point

 The IRD first parses its arguments (incoming and outgoing packet queues).
 It then launches the IRD routines and waits forever.
*/
int main(int argc, char const *argv[]) {
	int err;
	
	// Get arguments
	int queue_id = atoi(argv[1]);
	imd_queue_id = atoi(argv[2]);
	DEBUG("Started");

	// Launch time routine
	if ((err = pthread_create(&time_thread, NULL, time_routine, NULL))) {
		ERR("Unable to setup time thread", err);
	}
	DEBUG("Time thread created");

	/*
	// Launch receiving routine
	if ((err = pthread_create(&handle_thread, NULL, handle_routine, (void*) queue_id))) {
		ERR("Unable to setup receive thread", err);
	}
	DEBUG("Receive thread created");
	*/

#ifdef IPRP_MULTICAST
	// Launch subscribe routine
	if ((err = pthread_create(&si_thread, NULL, si_routine, NULL))) {
		ERR("Unable to setup sender interfaces thread", err);
	}
	DEBUG("Sender interfaces thread created");
#endif

	LOG("Receiver daemon successfully created");

	// Launch receiving routine
	handle_routine((void*) queue_id);
	
	LOG("Last man standing at the end of the apocalypse");
	return EXIT_FAILURE;
}
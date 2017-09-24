/**\file imd.c
 * iPRP Monitoring Daemon
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#define IPRP_FILE IMD_MAIN

#include <stdlib.h>
#include <pthread.h>

#include "imd.h"

/* Threads */
pthread_t time_thread;
pthread_t handle_thread;
pthread_t ird_handle_thread;
pthread_t as_thread;

extern list_t active_senders;

/**
 Moitoring daemon entry point

 The IMD first gets the numbers of the queues it has to monitor from its arguments.
 It then sets up and launches all the IMD routines and waits forever.
*/
int main(int argc, char const *argv[]) {
	int err;
	
	// Get arguments
	int queue_id = atoi(argv[1]);
	int ird_queue_id = atoi(argv[2]);
	DEBUG("Started");

	// Initialize active senders cache
	list_init(&active_senders);
	DEBUG("Receiver links list initialized");

	// Launch receiving routine
	if ((err = pthread_create(&time_thread, NULL, time_routine, NULL))) {
		ERR("Unable to setup monitoring thread", err);
	}
	DEBUG("Time thread created");

	/*
	// Launch receiving routine
	if ((err = pthread_create(&handle_thread, NULL, handle_routine, (void*) queue_id))) {
		ERR("Unable to setup monitoring thread", err);
	}
	DEBUG("Monitor thread created");
	*/

	// Launch IRD receiving routine
	if ((err = pthread_create(&ird_handle_thread, NULL, ird_handle_routine, (void*) ird_queue_id))) {
		ERR("Unable to setup IRD monitoring thread", err);
	}
	DEBUG("IRD monitor thread created");

	// Launch active senders routine
	if ((err = pthread_create(&as_thread, NULL, as_routine, NULL))) {
		ERR("Unable to setup active senders thread", err);
	}
	DEBUG("Active senders thread created");

	LOG("Monitoring daemon successfully created");

	// Launch receiving routine
	handle_routine((void*) queue_id);

	/* Should not reach this part */
	LOG("Last man standing at the end of the apocalypse");
	return EXIT_FAILURE;
}


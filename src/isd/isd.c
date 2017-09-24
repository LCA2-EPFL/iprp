/**\file isd.c
 * iPRP Sender Daemon
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#define IPRP_FILE ISD_MAIN

#include <errno.h>
#include <pthread.h>

#include "isd.h"
#include "peerbase.h"

/* Global variables */
int sockets[IPRP_MAX_INDS];
iprp_isd_peerbase_t pb = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.cond = PTHREAD_COND_INITIALIZER,
	.loaded = false
};

/* Function prototypes */
int create_socket();

/**
 Sender daemon entry point

 The ISD first gets its queue and peerbase path from its arguments.
 It then creates the sockets it will use to send iPRP packets.
 It finally launches the needed routines and waits forever.
*/
int main(int argc, char const *argv[]) {
	// Thread variables
	pthread_t pb_thread;
	pthread_t handle_thread;
	pthread_t time_thread;

	int err;
	
	// Get arguments
	int queue_id = atoi(argv[1]);
	char* base_path = argv[2];
	DEBUG("Started");

	// Create send sockets and configure multicast outgoing interface
	for (int i = 0; i < IPRP_MAX_INDS; ++i) {
		if ((sockets[i] = create_socket()) < 0) {
			ERR("Unable to setup socket", errno);
		}
	}
	DEBUG("Sockets created");

	// Launch time routine
	if ((err = pthread_create(&time_thread, NULL, time_routine, NULL))) {
		ERR("Unable to setup time thread", err);
	}
	DEBUG("Time thread created");

	// Launch cache routine
	if ((err = pthread_create(&pb_thread, NULL, pb_routine, base_path))) {
		ERR("Unable to setup peerbase thread", err);
	}
	DEBUG("Peerbase thread created");

	/*
	// Launch send routine
	if ((err = pthread_create(&handle_thread, NULL, handle_routine, (void*) queue_id))) {
		ERR("Unable to setup handle thread", err);
	}
	DEBUG("Handle thread created");
	*/
	
	LOG("Sender daemon successfully created");

	// Launch send routine
	handle_routine((void*) queue_id);

	/*
	// Join on threads (should not happen)
	void* return_value;
	if ((err = pthread_join(pb_thread, &return_value))) {
		ERR("Unable to join on peerbase thread", err);
	}
	ERR("Peerbase thread unexpectedly finished execution", (int) return_value);
	if ((err = pthread_join(handle_thread, &return_value))) {
		ERR("Unable to join on handle thread", err);
	}
	ERR("Handle thread unexpectedly finished execution", (int) return_value);
	if ((err = pthread_join(time_thread, &return_value))) {
		ERR("Unable to join on time thread", err);
	}
	ERR("Time thread unexpectedly finished execution", (int) return_value);
	*/
	
	LOG("Last man standing at the end of the apocalypse");
	return EXIT_FAILURE;
}

/**
 Creates an ISD socket
*/
int create_socket() {
	int sock;
	if ((sock = socket(IPRP_AF, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		return IPRP_ERR;
	}

#ifdef IPRP_MULTICAST
	uint8_t ttl = 255;
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
	bool loopback = false;
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback));
#endif

	return sock;
}

/*
void cleanup() {
	// TODO implement clean ISD shutdown
	nfq_destroy_queue(queue);
	nfq_close(handle);
}
*/
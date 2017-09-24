/**\file icd/activesenders.c
 * Active sender handler for the ICD side
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#define IPRP_FILE ICD_AS

#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>

#include "icd.h"
#include "activesenders.h"

extern time_t curr_time;
extern iprp_host_t this;

/* Function prototypes */
int get_active_senders(iprp_active_sender_t **senders);
void send_cap(iprp_active_sender_t *sender, int socket);
#ifdef IPRP_MULTICAST
int backoff();
// Function def in control.c
bool in_sender_ifaces(IPRP_INADDR group_addr, IPRP_INADDR src_addr);
#endif

/**
 Sends CAP messages to the active senders
 
 The active senders routine initializes the active sender file (to avoid reading an unexisting file).
 It then sets up a socket to send the CAP messages.
 Then periodically after a backoff period, it sends CAP messages to all active senders.
*/
void* as_routine(void* arg) {
	DEBUG("In routine");
	srand(curr_time);

	// Initialize active sender list
	activesenders_store(IPRP_AS_FILE, 0, NULL);
	DEBUG("Active senders list initialized");

	// Create sender socket
	int sendcap_socket;
	if ((sendcap_socket = socket(IPRP_AF, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		ERR("Unable to create send CAP socket", errno);
	}
	DEBUG("Socket created");

	while(true) {
		// Get active senders
		iprp_active_sender_t* senders;
		int count = get_active_senders(&senders);
		DEBUG("Active senders retrieved");

		// Send CAP messages
		for (int i = 0; i < count; ++i) {
		#ifdef IPRP_MULTICAST
			// Do not send CAP if ACK was received recently
			if (!in_sender_ifaces(senders[i].dest_addr, senders[i].src_addr)) {
				break;
			}
		#endif
			send_cap(&senders[i], sendcap_socket);			
			DEBUG("CAP sent");
		}
		LOG("All CAPs sent");

	#ifndef IPRP_MULTICAST
		sleep(ICD_T_CAP);
	#else
		sleep(ICD_T_CAP + backoff());
	#endif
	}
}

/**
 Reads active senders from file
*/
int get_active_senders(iprp_active_sender_t **senders) {
	int count;
	int err;
	if ((err = activesenders_load(IPRP_AS_FILE, &count, senders))) {
		ERR("Unable to get active senders", err);
	}

	return count;
}

/**
 Creates and sends a CAP message to a given sender
*/
void send_cap(iprp_active_sender_t *sender, int socket) {
	// Create message
	iprp_capmsg_t cap;
	cap.iprp_version = IPRP_VERSION;
	cap.src_addr = sender->src_addr;
	cap.dest_addr = sender->dest_addr;
#ifndef IPRP_MULTICAST
	cap.receiver = this;
#endif
	cap.inds = ind_match(&this, -1);
	cap.src_port = sender->src_port;
	cap.dest_port = sender->dest_port;

	// Create message wrapper
	iprp_ctlmsg_t msg;
	msg.secret = IPRP_CTLMSG_SECRET;
	msg.msg_type = IPRP_CAP;
	msg.message.cap_message = cap;

	// Send message
	IPRP_SOCKADDR addr;
	sockaddr_fill(&addr, sender->src_addr, IPRP_CTL_PORT);
	sendto(socket, (void*) &msg, sizeof(msg), 0, (struct sockaddr*) &addr, sizeof(addr));
}

#ifdef IPRP_MULTICAST
/**
 Computes the time to wait before sending the next round of CAPs
*/
int backoff() {
	srand(curr_time);
	double random = 0.0;
	double x = 2;
	while(random == 0.0 && x > 1) {
		random = rand();
		x = -log(random/(RAND_MAX)) / IPRP_BACKOFF_LAMBDA;
	}
	return (1-x)*IPRP_BACKOFF_D;
}
#endif
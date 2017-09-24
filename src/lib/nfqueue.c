/**\file nfqueue.c
 * NFQueue functions
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "global.h"

/**
 Sets up the given queue to handle its packet from the given callback
*/
int queue_setup(iprp_queue_t *nfq, int queue_id, nfq_callback *callback) {
	// Setup nfqueue
	nfq->handle = nfq_open();
	if (!nfq->handle) {
		ERR("Unable to open queue handle", IPRP_ERR_NFQUEUE);
	}
	if (nfq_unbind_pf(nfq->handle, IPRP_AF) < 0) {
		ERR("Unable to unbind IP protocol from queue handle", IPRP_ERR_NFQUEUE);
	}
	if (nfq_bind_pf(nfq->handle, IPRP_AF) < 0) {
		ERR("Unable to bind IP protocol to queue handle", IPRP_ERR_NFQUEUE);
	}
	nfq->queue = nfq_create_queue(nfq->handle, queue_id, callback, NULL);
	if (!nfq->queue) {
		ERR("Unable to create queue", IPRP_ERR_NFQUEUE);
	}
	if (nfq_set_queue_maxlen(nfq->queue, IPRP_NFQUEUE_MAX_LENGTH) == -1) {
		ERR("Unable to set queue max length", IPRP_ERR_NFQUEUE);
	}
	if (nfq_set_mode(nfq->queue, NFQNL_COPY_PACKET, 0xffff) == -1) {
		ERR("Unable to set queue mode", IPRP_ERR_NFQUEUE);
	}
	nfq->fd = nfq_fd(nfq->handle);
    int enable = 1;
    setsockopt(nfq->fd, SOL_NETLINK, NETLINK_NO_ENOBUFS, enable, sizeof(enable));

	return 0;
}

/**
 Handles the next packet from the queue
*/
int get_and_handle(struct nfq_handle *handle, int queue_fd) {
	int bytes;
	char buf[IPRP_PKTBUF_SIZE];

	// Get packet from queue
	if ((bytes = recv(queue_fd, buf, IPRP_PKTBUF_SIZE, 0)) == -1) {
		if (errno == ENOBUFS) { // We are dropping packets, ignore
			//return IPRP_ERR_UNKNOWN;
		}
		return IPRP_ERR;
	} else if (bytes == 0) {
		return IPRP_ERR_EMPTY;
	}

	// Handle packet
	if (nfq_handle_packet(handle, buf, bytes) == -1) {
		//ERR("Error while handling packet", err);
	}
	return 0;
}

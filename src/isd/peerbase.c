/**\file isd/peerbase.c
 * Peerbase handler on the ISD side
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#define IPRP_FILE ISD_PB

#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>

#include "isd.h"
#include "peerbase.h"

extern iprp_isd_peerbase_t pb;
extern int sockets[];

/**
 Caches the peerbase

 The routine periodically loads the peerbase from file and updates its cache.
 The first time it does so, it signals the handling thread that it can begin to send packets.
*/
void* pb_routine(void *arg) {
	// Get argument
	char* base_path = arg;
	DEBUG("In routine");

	while(true) {
		iprp_peerbase_t temp;

		// Load peerbase from file
		int err = peerbase_load(base_path, &temp);
		if (err) {
			ERR("Unable to load peerbase", err);
		}
		DEBUG("Peerbase loaded");

		// Update
		pthread_mutex_lock(&pb.mutex);
		pb.base = temp;
		pthread_mutex_unlock(&pb.mutex);
		DEBUG("Peerbase cached");

	#ifdef IPRP_MULTICAST
		// Update sockets according to loaded peerbase (no need to lock nor correct dropped sockets) TODO ipv6
		for (int i = 0; i < pb.base.host.nb_ifaces; ++i) {
			iprp_iface_t iface = pb.base.host.ifaces[i];
			if (setsockopt(sockets[iface.ind], IPPROTO_IP, IP_MULTICAST_IF, &iface.addr, sizeof(iface.addr)) == -1) {
				ERR("Unable to set outgoing interface", errno);
			}
		}
		DEBUG("Sockets updated");
	#endif

		// Allow launching of send routine
		pb.loaded = true;
		pthread_cond_signal(&pb.cond);

		LOG("Peerbase cached");
		sleep(ISD_T_PB_CACHE);
	}
}
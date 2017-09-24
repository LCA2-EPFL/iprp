#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "icd.h"
#include "peerbase.h"

int main(int argc, char const *argv[]) {
	if (argc != 5) {
		printf("Usage: iprptest host port num_packets time\n");
		return EXIT_FAILURE;
	}

	// Parameter parsing
	IPRP_INADDR remote_addr;
	inet_pton(IPRP_AF, argv[1], &remote_addr);
	uint16_t remote_port = atoi(argv[2]);

	int num_packets = atoi(argv[3]);
	int time_period = atoi(argv[4]);

	// Open directory
	char* dir = "files/";
	DIR *dirfd;
	if ((dirfd = opendir(dir)) == NULL) {
		printf("Unable to open files directory\n");
		return EXIT_FAILURE;
	}

	// Traverse directory
	struct dirent *dp;
	while((dp = readdir(dirfd)) != NULL) {
		if (strncmp(dp->d_name, "base_", 5) == 0) { // This is a peerbase file
			// Load peerbase
			char filename[256];
			snprintf(filename, 256, "%s/%s", dir, dp->d_name);
			iprp_peerbase_t base;
			if (peerbase_load(filename, &base) == -1) {
				return EXIT_FAILURE;
			}

			if (COMPARE_ADDR(base.link.dest_addr, remote_addr)
				&& base.link.dest_port == remote_port) {
				// This is the right peerbase
				printf("IND  Local IP         Remote IP\n");
				for (int i = 0; i < 16; ++i) {
					if (base.inds & (1 << i)) {
						char local_ip[IPRP_ADDRSTRLEN];
						char remote_ip[IPRP_ADDRSTRLEN];
						inet_ntop(IPRP_AF, &base.host.addrs[i], local_ip, IPRP_ADDRSTRLEN);
						inet_ntop(IPRP_AF, &base.dest_addr[i], remote_ip, IPRP_ADDRSTRLEN);
					#ifndef IPRP_IPV6
						printf("0x%x  %-15s  %-15s\n", i, local_ip, remote_ip);
					#else
						printf("0x%x  %-25s  %-25s\n", i, local_ip, remote_ip);
					#endif
					}
				}
				return EXIT_SUCCESS;
			}
		}
	}

	// We didn't find an active connection
	printf("No connection found, sending probe packets...\n");

	// Setup socket
	int stats_socket;
	if ((stats_socket = socket(IPRP_AF, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("Unable to create socket (%d)", errno);
	}
	IPRP_SOCKADDR ctl_addr;
	sockaddr_fill(&ctl_addr, remote_addr, IPRP_CTL_PORT);

	// Create opening message
	iprp_ctlmsg_t on_msg;
	on_msg.secret = IPRP_CTLMSG_SECRET;
	on_msg.msg_type = IPRP_DUMMY_ON;
	on_msg.message.stats_message.port = remote_port;

	// Send opening message
	if (sendto(stats_socket, &on_msg, sizeof(on_msg), 0, &ctl_addr, sizeof(ctl_addr)) == -1) {
		printf("Unable to send to receiver (%d)", errno);
		return EXIT_FAILURE;
	}
	printf("Temporary connection established\n");

	// Setup dummy messages
	char msg[100];
	IPRP_SOCKADDR msg_addr;
	sockaddr_fill(&msg_addr, remote_addr, remote_port);

	// Send dummy messages
	for (int i = 0; i < num_packets; ++i) {
		sleep(time_period);

		snprintf(msg, 99, "Dummy message %d", i);
		sendto(stats_socket, &msg, sizeof(msg), 0, &msg_addr, sizeof(msg_addr));
		printf("Message %d sent\n", i);
	}

	// Create closing message
	iprp_ctlmsg_t off_msg;
	off_msg.secret = IPRP_CTLMSG_SECRET;
	off_msg.msg_type = IPRP_DUMMY_OFF;
	off_msg.message.stats_message.port = remote_port;

	// Send closing message
	if (sendto(stats_socket, &off_msg, sizeof(off_msg), 0, &ctl_addr, sizeof(ctl_addr)) == -1) {
		printf("Unable to send to receiver (%d)", errno);
		return EXIT_FAILURE;
	}
	printf("Connection closed\n");

	// Close socket
	close(stats_socket);

	return EXIT_SUCCESS;
}
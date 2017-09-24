#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "icd.h"
#include "ird.h"

int main(int argc, char const *argv[]) {
	if (argc != 3) {
		printf("Usage: iprpsendstats host port\n");
		return EXIT_FAILURE;
	}

	// Parameter parsing
	IPRP_INADDR remote_addr;
	inet_pton(IPRP_AF, argv[1], &remote_addr);
	uint16_t remote_port = atoi(argv[2]);

	// Setup receiving socket
	int stats_socket;
	if ((stats_socket = socket(IPRP_AF, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("Unable to create socket (%d)", errno);
	}
	IPRP_SOCKADDR local;
	IPRP_INADDR any = IPRP_INADDR_ANY_INIT;
	sockaddr_fill(&local, any, IPRP_STATS_PORT);
	if (bind(stats_socket, &local, sizeof(local)) == -1) {
		printf("Unable to bind to stats port (%d)", errno);
		return EXIT_FAILURE;
	}

	// Create control message
	iprp_ctlmsg_t msg;
	msg.secret = IPRP_CTLMSG_SECRET;
	msg.msg_type = IPRP_STATS;
	msg.message.stats_message.port = remote_port;

	// Send control message
	IPRP_SOCKADDR addr;
	sockaddr_fill(&addr, remote_addr, IPRP_CTL_PORT);
	if (sendto(stats_socket, &msg, sizeof(msg), 0, &addr, sizeof(addr)) == -1) {
		printf("Unable to send to receiver (%d)", errno);
		return EXIT_FAILURE;
	}

	// Receive stats file
	printf("About to receive\n");
	iprp_receiver_link_t link;
	if (recv(stats_socket, &link, sizeof(iprp_receiver_link_t), 0) < sizeof(iprp_receiver_link_t)) {
		printf("No connection found\n");
	} else {
		// Print stats file
		printf("IND  Pkts  Acc.  Wrong Last seen\n");
		char last_seen_buf[26];
		for (int i = 0; i < 16; ++i) {
			if (link.stats.pkts[i] > 0) {
			    strftime(last_seen_buf, 26, "%Y-%m-%d %H:%M:%S", localtime(&link.stats.last_seen[i]));
				printf("0x%x  %4d  %4d  %4d  %s\n", i, link.stats.pkts[i], link.stats.acc[i], link.stats.wrong[i], last_seen_buf);
			}
		}
	}

	// Close socket
	close(stats_socket);

	return EXIT_SUCCESS;
}

void print_time(char* buffer, time_t t) {
}
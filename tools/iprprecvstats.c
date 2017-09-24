#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "ird.h"

int main(int argc, char const *argv[]) {
	if (argc != 3) {
		printf("Usage: iprprecvstats host port\n");
		return EXIT_FAILURE;
	}

	// Parameter parsing
	IPRP_INADDR remote_addr;
	inet_pton(IPRP_AF, argv[1], &remote_addr);
	uint16_t remote_port = atoi(argv[2]);

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
		if (strncmp(dp->d_name, "stats_", 5) == 0) { // This is a stats file
			// Load stats file
			char filename[256];
			snprintf(filename, 256, "%s/%s", dir, dp->d_name);
			iprp_receiver_link_t link;
			if (stats_load(filename, &link) == -1) {
				return EXIT_FAILURE;
			}

			if (COMPARE_ADDR(link.src_addr, remote_addr)
				&& link.src_port == remote_port) {
				// This is the right stats file
				printf("IND  Pkts  Acc.  Wrong Last seen\n");

				char last_seen_buf[26];
				for (int i = 0; i < 16; ++i) {
					if (link.stats.pkts[i] > 0) {
					    strftime(last_seen_buf, 26, "%Y-%m-%d %H:%M:%S", localtime(&link.stats.last_seen[i]));
						printf("0x%x  %4d  %4d  %4d  %s\n", i, link.stats.pkts[i], link.stats.acc[i], link.stats.wrong[i], last_seen_buf);
					}
				}
				return EXIT_SUCCESS;
			}
		}
	}

	// We didn't find an active connection
	printf("No connection found\n");
	return EXIT_SUCCESS;
}
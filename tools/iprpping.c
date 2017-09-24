#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "peerbase.h"

int main(int argc, char const *argv[]) {
	if (argc != 4) {
		printf("Usage: iprpping host count packet_size\n");
		return EXIT_FAILURE;
	}

	// Parameter parsing
	IPRP_INADDR remote_addr;
	inet_pton(IPRP_AF, argv[1], &remote_addr);

	int count = atoi(argv[2]);
	int size = atoi(argv[3]);

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

			if (COMPARE_ADDR(base.link.dest_addr, remote_addr)) {
				// This is the right peerbase
				for (int i = 0; i < 16; ++i) {
					if (base.inds & (1 << i)) {
						char remote_ip[IPRP_ADDRSTRLEN];
						inet_ntop(IPRP_AF, &base.dest_addr[i], remote_ip, IPRP_ADDRSTRLEN);

						char shell[256];
						snprintf(shell, 255, "ping -c %d -s %d -n -q %s", count, size, remote_ip);
						system(shell);
						printf("\n");
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
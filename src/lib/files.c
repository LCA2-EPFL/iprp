/**\file files.c
 * Disk management functions
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "activesenders.h"
#include "peerbase.h"
#ifdef IPRP_MULTICAST
 #include "senderifaces.h"
#endif
#include "ird.h"

/**
 Stores the given peerbase to the given file
*/
int peerbase_store(const char* path, iprp_peerbase_t *base) {
	if (path == NULL | base == NULL) return IPRP_ERR_NULLPTR;

	// Save file
	FILE* writer = fopen(path, "w");
	if (!writer) {
		ERR("Unable to write to peerbase file", errno);
	}

	fwrite(base, sizeof(iprp_peerbase_t), 1, writer);

	fflush(writer);		

	fclose(writer);

	return 0;
}

/**
 Loads the peerbase from the given file
*/
int peerbase_load(const char *path, iprp_peerbase_t *base) {
	if (path == NULL || base == NULL) return IPRP_ERR_NULLPTR;

	FILE* reader = fopen(path, "r");
	if (!reader) {
		ERR("Unable to read peerbase file", errno);
	}

	fread(base, sizeof(iprp_peerbase_t), 1, reader);

	fclose(reader);

	return 0;
}

/**
 Stores the given active senders to the given file
*/
void activesenders_store(const char* path, const int count, const iprp_active_sender_t* senders) {
	// Get file descriptor
	FILE* writer = fopen(path, "w");
	if (!writer) {
		ERR("Unable to open active senders file", errno);
	}

	// Write entry count
	fwrite(&count, sizeof(int), 1, writer);

	// Write entries
	if (count > 0) {
		fwrite(senders, sizeof(iprp_active_sender_t), count, writer);
	}

	// Cleanup write
	fflush(writer);
	fclose(writer);
}

/**
 Retrieves the active senders list from the given file
*/
int activesenders_load(const char *path, int* count, iprp_active_sender_t** senders) {
	if (!count || !senders) return IPRP_ERR_NULLPTR;

	// Get file descriptor
	FILE* reader = fopen(path, "r");
	if (!reader) {
		ERR("Unable to open active senders file", errno);
	}

	// Read entry count
	fread(count, sizeof(int), 1, reader);

	// Read entries
	if (count > 0) {
		*senders = calloc(*count, sizeof(iprp_active_sender_t));
		fread(*senders, sizeof(iprp_active_sender_t), *count, reader);
	}

	// Cleanup read
	fclose(reader);

	return 0;
}

#ifdef IPRP_MULTICAST
/**
 Stores the given sender interfaces to the given file
*/
void senderifaces_store(const char* path, const int count, const iprp_sender_ifaces_t* senders) {
	// Get file descriptor
	FILE* writer = fopen(path, "w");
	if (!writer) {
		ERR("Unable to open sender interfaces file", errno);
	}

	// Write entry count
	fwrite(&count, sizeof(int), 1, writer);

	// Write entries
	if (count > 0) {
		fwrite(senders, sizeof(iprp_sender_ifaces_t), count, writer);
	}

	// Cleanup write
	fflush(writer);
	fclose(writer);
}

/**
 Retrieves the sender interfaces list from the given file
*/
int senderifaces_load(const char *path, int* count, iprp_sender_ifaces_t** senders) {
	if (!count || !senders) return IPRP_ERR_NULLPTR;

	// Get file descriptor
	FILE* reader = fopen(path, "r");
	if (!reader) {
		ERR("Unable to open sender interfaces file", errno);
	}

	// Read entry count
	fread(count, sizeof(int), 1, reader);

	// Read entries
	if (count > 0) {
		*senders = calloc(*count, sizeof(iprp_sender_ifaces_t));
		fread(*senders, sizeof(iprp_sender_ifaces_t), *count, reader);
	}

	// Cleanup read
	fclose(reader);

	return 0;
}
#endif

/**
 Stores the given statistics to the given file
*/
int stats_store(const char* path, iprp_receiver_link_t *stats) {
	if (path == NULL | stats == NULL) return IPRP_ERR_NULLPTR;

	// Save file
	FILE* writer = fopen(path, "w");
	if (!writer) {
		ERR("Unable to write to stats file", errno);
	}

	fwrite(stats, sizeof(iprp_receiver_link_t), 1, writer);

	fflush(writer);		

	fclose(writer);

	return 0;
}

/**
 Loads the statistics from the given file
*/
int stats_load(const char *path, iprp_receiver_link_t *stats) {
	if (path == NULL || stats == NULL) return IPRP_ERR_NULLPTR;

	FILE* reader = fopen(path, "r");
	if (!reader) {
		ERR("Unable to read stats file", errno);
	}

	fread(stats, sizeof(iprp_receiver_link_t), 1, reader);

	fclose(reader);

	return 0;
}
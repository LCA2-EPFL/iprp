/**\file icd/ports.c
 * Monitored ports handling
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#define IPRP_FILE ICD_PORTS

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "icd.h"

/* Function prototypes */
size_t get_monitored_ports(uint16_t **table);
bool find_port_in_array(uint16_t port, uint16_t* array, size_t array_size);
bool find_port_in_list(uint16_t port, list_t *list);
void iptables_rule(uint16_t port, uint16_t queue_num, bool create);
pid_t ird_launch(uint16_t queue_num, uint16_t imd_queue_num);
pid_t imd_launch(uint16_t queue_num, uint16_t ird_queue_num);
void proc_shutdown(pid_t pid);

/**
 Caches the monitored ports file and the IMD and IRD

 The routine periodically reads the monitored ports file.
 It updates its internal monitored ports list accordingly.
 If needed, it then launches or shuts down the IRD and IMD.
*/
void* ports_routine(void* arg) {
	iprp_icd_recv_queues_t *queue_nums = (iprp_icd_recv_queues_t *) arg;
	DEBUG("In routine");

	// Initialize monitored ports cache
	list_t monitored_ports;
	list_init(&monitored_ports);
	DEBUG("Monitored ports list initialized");
	
	// Ports list caching
	bool receiver_active = false;
	pid_t ird_pid, imd_pid;
	while(true) {
		// Get list of ports
		uint16_t *new_ports;
		size_t num_ports = get_monitored_ports(&new_ports);
		DEBUG("Got %zu monitored ports from list", num_ports);

		// Close expired ports
		list_elem_t *iterator = monitored_ports.head;
		while(iterator != NULL) {
			uint16_t port = (uint16_t) iterator->elem;
			if (!find_port_in_array(port, new_ports, num_ports)) {
				// Delete list item, iptables rule
				list_elem_t *next = iterator->next;
				list_delete(&monitored_ports, iterator);
				iterator = next;

				// Delete iptables rule
				iptables_rule(port, queue_nums->imd, false);
				DEBUG("Port %u deleted from list", port);
			} else {
				iterator = iterator->next;
				DEBUG("Port %u remains in list", port);
			}
		}
		DEBUG("Closing phase over");

		// Open new ports
		for (int i = 0; i < num_ports; ++i) {
			if (!find_port_in_list(new_ports[i], &monitored_ports)) {
				// Create list item
				list_append(&monitored_ports, (void*) new_ports[i]);

				// Create iptables rule
				iptables_rule(new_ports[i], queue_nums->imd, true);
				DEBUG("Port %u added to list", new_ports[i]);
			} else {
				DEBUG("Port %u already in list", new_ports[i]);
			}
		}
		DEBUG("Opening phase over");

		// Create or delete IRD/IMD
		if (receiver_active && (list_size(&monitored_ports) == 0)) {
			// Shutdown IMD and IRD
			proc_shutdown(ird_pid);
			proc_shutdown(imd_pid);
			receiver_active = false;
			DEBUG("IRD and IMD shutdown");
		} else if (!receiver_active && list_size(&monitored_ports) > 0) {
			// Launch receiver deamon
			ird_pid = ird_launch(queue_nums->ird, queue_nums->ird_imd);
			if (ird_pid == -1) {
				ERR("Unable to create receiver deamon", errno);
			}
			DEBUG("IRD launched");

			// Launch monitoring deamon
			imd_pid = imd_launch(queue_nums->imd, queue_nums->ird_imd);
			if (imd_pid == -1) {
				ERR("Unable to create monitoring deamon", errno);
			}
			DEBUG("IMD launched");

			receiver_active = true;
		}

		LOG("Ports update phase complete");

		sleep(ICD_T_PORTS);
	}
}

/**
 Reads the monitored ports from the file
*/
size_t get_monitored_ports(uint16_t **table) {
	// Get file descriptor
	FILE* ports_file = fopen(IPRP_PORTS_FILE, "r");
	if (!ports_file) {
		ERR("Unable to open ports file", errno);
	}

	// Read file into ports table
	int bytes;
	uint16_t ports[IPRP_MAX_MONITORED_PORTS];
	int num_ports;
	for (int i = 0; i < IPRP_MAX_MONITORED_PORTS; ++i) {
		bytes = fscanf(ports_file, "%hu", &ports[i]);
		if (bytes == EOF) {
			num_ports = i;
			break;
		}
	}
	if (bytes != EOF) {
		num_ports = IPRP_MAX_MONITORED_PORTS;
	}

	// Copy table into return value
	*table = calloc(num_ports, sizeof (uint16_t));
	for (int i = 0; i < num_ports; ++i) {
		(*table)[i] = ports[i];
	}

	return num_ports;
}

/**
 Returns whether the given port is contained in the given array
*/
bool find_port_in_array(uint16_t port, uint16_t* array, size_t array_size) {
	for (int i = 0; i < array_size; ++i) {
		if (port == array[i]) {
			return true;
		}
	}
	return false;
}

/**
 Returns whether the given list contains the given port
*/
bool find_port_in_list(uint16_t port, list_t *list) {
	list_elem_t *iterator = list->head;
	while(iterator != NULL) {
		uint16_t list_port = (uint16_t) iterator->elem;
		if (port == list_port) {
			return true;
		}
		iterator = iterator->next;
	}
	return false;
}

/**
 Creates or deletes an iptables rule redirecting all traffic through the given port to the given queue
*/
void iptables_rule(uint16_t port, uint16_t queue_num, bool create) {
	char buf[100];
#ifndef IPRP_IPV6
	snprintf(buf, 100, "sudo iptables -t mangle -%s PREROUTING -p udp --dport %d -j NFQUEUE --queue-num %d", create ? "A" : "D", port, queue_num);
#else
	snprintf(buf, 100, "sudo ip6tables -t mangle -%s PREROUTING -p udp --dport %d -j NFQUEUE --queue-num %d", create ? "A" : "D", port, queue_num);
#endif
	system(buf);
}

/**
 Launches the IRD
*/
pid_t ird_launch(uint16_t queue_num, uint16_t imd_queue_num) {
	pid_t pid = fork();
	if (!pid) { // Child side
		// Create NFqueue
		iptables_rule(IPRP_DATA_PORT, queue_num, true);
		DEBUG("NFQueue created for IRD");
		
		// Launch receiver
		char queue_id_str[16];
		sprintf(queue_id_str, "%d", queue_num);
		char imd_queue_id_str[16];
		sprintf(imd_queue_id_str, "%d", imd_queue_num);
		if (execl(IPRP_IRD_BINARY_LOC, "ird", queue_id_str, imd_queue_id_str, NULL) == -1) {
			ERR("Unable to launch receiver deamon", errno);
		}
	} else {
		// Parent side
		return pid;
	}
}

/**
 Launches the IMD
*/
pid_t imd_launch(uint16_t queue_num, uint16_t ird_queue_num) {
	pid_t pid = fork();
	if (!pid) { // Child side
		// Launch receiver
		char queue_id_str[16];
		sprintf(queue_id_str, "%d", queue_num);
		char ird_queue_id_str[16];
		sprintf(ird_queue_id_str, "%d", ird_queue_num);
		if (execl(IPRP_IMD_BINARY_LOC, "imd", queue_id_str, ird_queue_id_str, NULL) == -1) {
			ERR("Unable to launch monitoring deamon", errno);
		}
	} else {
		// Parent side
		return pid;
	}
}

void proc_shutdown(pid_t pid) {
	char shell[16];
	snprintf(shell, 16, "kill -9 %d", pid);
	if (system(shell) == -1) {
		ERR("Unable to shutdown process", errno);
	}
}
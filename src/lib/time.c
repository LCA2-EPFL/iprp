/**\file time.c
 * Time functions
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include "global.h"

/* Current time cache */
time_t curr_time = 0;

/**
 Periodically samples time (to avoid repeated syscalls in handling routines)
*/
void *time_routine(void* arg) {
	while(true) {
		curr_time = time(NULL);
		sleep(1);
	}
}
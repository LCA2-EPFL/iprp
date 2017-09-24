/**\file imd.h
 * Header file for IMD
 * 
 * \author Loic Ottet (loic.ottet@epfl.ch)
 */

#ifndef __IPRP_IMD_
#define __IPRP_IMD_

#include "global.h"
#include "activesenders.h"

/* Thread routines */
void* handle_routine(void* arg);
void* ird_handle_routine(void* arg);
void* as_routine(void* arg);

#endif /* __IPRP_IMD_ */
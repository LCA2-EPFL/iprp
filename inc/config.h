#ifndef __IPRP_CONFIG_
#define __IPRP_CONFIG_

// Timers are given in seconds

/* Global definitions */
#define IPRP_CTL_PORT 			1000		// Port used for control messages
#define IPRP_DATA_PORT			1001		// Port used for iPRP messages
#define IPRP_STATS_PORT			1002		// Port used for stats messages
#define IPRP_ISD_BINARY_LOC		"bin/isd"	// Location of the ISD binary
#define IPRP_IRD_BINARY_LOC		"bin/ird"	// Location of the IRD binary
#define IPRP_IMD_BINARY_LOC		"bin/imd"	// Location of the IMD binary
#define IPRP_PORTS_FILE			"ports.txt"	// Location of the monitored ports file

/* Control daemon */
#define ICD_T_CAP				3			// Capability message sending interval
#define ICD_T_PB_CACHE			3			// Peerbases store interval
#define ICD_T_PB_EXP			60			// Peerbase expiry time
#define ICD_T_PORTS				10			// Monitored ports file load interval
#ifdef IPRP_MULTICAST
 #define ICD_T_SI_CACHE			3			// Sender interfaces store interval (multicast only)
 #define ICD_T_SI_EXP			60			// Sender interfaces expiry time (multicast only)
#endif

/* Monitoring daemon */
#define IMD_T_AS_CACHE			3			// Active senders store interval
#define IMD_T_AS_EXP			120			// Active sender expiry time

/* Receiving daemon */
#define IRD_T_CLEANUP			5			// Receiver link cleanup interval
#define IRD_T_EXP				120			// Receiver link expiry time
#ifdef IPRP_MULTICAST
 #define IRD_T_SI_CACHE			3			// Sender interfaces load interval (multicast only)
#endif

/* Sender daemon */
#define ISD_T_PB_CACHE			3			// Peerbases load interval
#define ISD_T_ALLOW				2			// Interval at which regular messages are let through (multicast only)

#endif /* __IPRP_CONFIG_ */
#ifndef __IPRP_IPV6_
#define __IPRP_IPV6_

#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/ip.h>
//#include <linux/ipv6.h>

#ifndef IPRP_IPV6
	#define IPRP_INADDR struct in_addr
	#define IPRP_SOCKADDR struct sockaddr_in
	#define IPRP_SOCKADDR_ADDR(sa) ((sa).sin_addr)
	#define IPRP_AF AF_INET
	#define IPRP_INADDR_ANY_INIT { INADDR_ANY }
	#define COMPARE_ADDR(x, y) ((x).s_addr == (y).s_addr)
	#define IPRP_ADDRSTRLEN INET_ADDRSTRLEN
	#define IPRP_IPHDR struct iphdr
	#define HEADER_TO_ADDR(dest, addr) (dest.s_addr = addr)
	#define ADDR_TO_HEADER(dest, addr) (dest = addr.s_addr)
#else
	struct ipv6hdr {
		uint8_t			version;
		uint8_t			flow_lbl[3];
		uint16_t		payload_len;
		uint8_t			nexthdr;
		uint8_t			hop_limit;
		struct in6_addr	saddr;
		struct in6_addr	daddr;
	};
	#define IPRP_INADDR struct in6_addr
	#define IPRP_SOCKADDR struct sockaddr_in6
	#define IPRP_SOCKADDR_ADDR(sa) ((sa).sin6_addr)
	#define IPRP_AF AF_INET6
	#define IPRP_INADDR_ANY_INIT in6addr_any
	#define COMPARE_ADDR(x, y) ( \
		(x).s6_addr[0] == (y).s6_addr[0] \
		&& (x).s6_addr[1] == (y).s6_addr[1] \
		&& (x).s6_addr[2] == (y).s6_addr[2] \
		&& (x).s6_addr[3] == (y).s6_addr[3] \
		&& (x).s6_addr[4] == (y).s6_addr[4] \
		&& (x).s6_addr[5] == (y).s6_addr[5] \
		&& (x).s6_addr[6] == (y).s6_addr[6] \
		&& (x).s6_addr[7] == (y).s6_addr[7] \
		&& (x).s6_addr[8] == (y).s6_addr[8] \
		&& (x).s6_addr[9] == (y).s6_addr[9] \
		&& (x).s6_addr[10] == (y).s6_addr[10] \
		&& (x).s6_addr[11] == (y).s6_addr[11] \
		&& (x).s6_addr[12] == (y).s6_addr[12] \
		&& (x).s6_addr[13] == (y).s6_addr[13] \
		&& (x).s6_addr[14] == (y).s6_addr[14] \
		&& (x).s6_addr[15] == (y).s6_addr[15] \
	)
	#define IPRP_ADDRSTRLEN INET6_ADDRSTRLEN
	#define IPRP_IPHDR struct ipv6hdr
	#define HEADER_TO_ADDR(dest, addr) (dest = addr)
	#define ADDR_TO_HEADER(dest, addr) (dest = addr)
#endif

#endif /* __IPRP_IPV6_ */
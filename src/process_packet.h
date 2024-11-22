#include "traffic_anon.h"
#ifndef PROCESS_PACKET_H
#define PROCESS_PACKET_H

#include <rte_hash.h>
#include <rte_jhash.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_lcore.h>
#include <arpa/inet.h>


/* Constants */
#define MAX_SUBNETS 256
//#define MAX_CORES 100

/* IP data structures */
extern struct in_addr net_list [MAX_SUBNETS];
extern struct in6_addr net_listv6 [MAX_SUBNETS];
extern int net_mask [MAX_SUBNETS];
extern int net_maskv6 [MAX_SUBNETS];
extern int tot_nets;
extern int tot_netsv6;


/* Functions */
void process_packet    (struct rte_mbuf * packet, out_interface_sett interface_setting, int id, int core);
void process_packet_eth (struct rte_mbuf * packet, out_interface_sett interface_setting, struct timespec);
void process_packet_ip (struct rte_mbuf * packet,  out_interface_sett interface_setting, int id, int core, struct timespec);
void process_packet_init(int);


/* Utility Function to print MAC addresses */
static inline void
print_ether_addr(const char *what, struct rte_ether_addr *eth_addr)
{
    char buf[RTE_ETHER_ADDR_FMT_SIZE];
    rte_ether_format_addr(buf, RTE_ETHER_ADDR_FMT_SIZE, eth_addr);
    printf("%s%s", what, buf);
}

#endif //PROCESS_PACKET_H

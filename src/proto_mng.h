#ifndef PROTO_MNG_H
#define PROTO_MNG_H

#include <rte_hash.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <sys/time.h>
#include <time.h>
#include <rte_malloc.h>


/* Variables */
extern hash_struct flow_db;


/* Functions for protocols */
void proto_init(int nb_sys_cores);
void multiplexer_proto(struct rte_ipv4_hdr * ipv4_header, struct rte_ipv6_hdr * ipv6_header, struct rte_mbuf * packet, int core, struct timespec tp, int id, out_interface_sett, crypto_ip *, int);
void dnsEntry (struct rte_mbuf * packet, int protocol, struct rte_ipv4_hdr * ipv4_header, struct rte_ipv6_hdr * ipv6_header, flow newPacket, hash_struct *flow_db, int k_anon, int k_delta, crypto_ip *, int, int);
void remove_dnsquery_name (char * buff);
void remove_dns_name (struct rte_mbuf * packet, ret_info info);
void remove_payload(struct rte_mbuf * packet, size_t offset);

/* Table Functions */
void table_init(hash_struct *);
int table_add(hash_struct *flow_db, flow flow_recv, char * name, int k_anon, int k_delta);



#endif //PROTO_MNG_H



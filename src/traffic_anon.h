#ifndef TRAFFIC_ANON_H
#define TRAFFIC_ANON_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <libgen.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>    //you know what this is for
#include <arpa/inet.h> //inet_addr , inet_ntoa , ntohs etc
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_tailq.h>
#include <rte_errno.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_log.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_string_fns.h>
#include <rte_cycles.h>
#include <rte_atomic.h>
#include <rte_version.h>

#include "param.h"
#include "defines.h"
#include "struct.h"

#include "ini.h"
#include "rijndael.h"
#include "crypto_ip.h"
#include "hash_calculator.h"
#include "ip_utils.h"
#include "lru.h"

#include "proto_finder.h"
#include "proto_mng.h"
#include "http_mng.h"
#include "dns_mng.h"
#include "flow_mng.h"
#include "ext-ip_mng.h"
#include "tls_mng.h"
#include "process_packet.h" 

/* Macros and constants */
#define FATAL_ERROR(fmt, args...) rte_exit(EXIT_FAILURE, fmt "\n", ##args)

extern out_interface_sett out_interface [MAX_INTERFACES];
extern in_interface_sett in_interface [MAX_INTERFACES];

/* Function prototypes */
static int main_loop(__attribute__((unused)) void * arg);
static void sig_handler(int signo);
static void init_port(int i);
static int parse_args(int argc, char **argv);
static int parse_ini(void* user, const char* section, const char* name,
                   const char* value);

typedef struct time_stat
{
	int n_pkt;
	int sum_time;
}time_stat;


/* Struct for devices configuration for const defines see rte_ethdev.h */
static struct rte_eth_conf port_conf = {
    .rxmode = {
        .mq_mode = RTE_ETH_MQ_RX_RSS, /* Enable RSS */
    },
    .txmode = {
        .mq_mode = RTE_ETH_MQ_TX_NONE,
    },
    .rx_adv_conf = {
        .rss_conf = {
            .rss_key_len = 40, /* and the seed length. */
            .rss_hf = (RTE_ETH_RSS_NONFRAG_IPV4_TCP | RTE_ETH_RSS_NONFRAG_IPV4_UDP) , /* RSS mask*/
        }    
    }
};


/* Struct for configuring each rx queue. These are default values */
static const struct rte_eth_rxconf rx_conf = {
    .rx_thresh = {
        .pthresh = 8,   /* Ring prefetch threshold */
        .hthresh = 8,   /* Ring host threshold */
        .wthresh = 4,   /* Ring writeback threshold */
    },
    .rx_free_thresh = 32,    /* Immediately free RX descriptors */
};

/* Struct for configuring each tx queue. These are default values */
static const struct rte_eth_txconf tx_conf = {
    .tx_thresh = {
        .pthresh = 36,  /* Ring prefetch threshold */
        .hthresh = 0,   /* Ring host threshold */
        .wthresh = 0,   /* Ring writeback threshold */
    },
    .tx_free_thresh = 0,    /* Use PMD default values */
    .offloads = 0,  /* IMPORTANT for vmxnet3, otherwise it won't work */
    .tx_rs_thresh = 0,      /* Use PMD default values */
};




#endif //TRAFFIC_ANON_H

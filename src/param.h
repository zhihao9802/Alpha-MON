#ifndef PARAM_H
#define PARAM_H

#define MEMPOOL_NAME "MEM_POOL"
#define MEMPOOL_NAME2 "MEM_POOL2"
#define MEMPOOL_CACHE_SZ 250      // MUST BE 0 WITH SCHED_DEADLINE, otherwise the driver wil crash !!
#define RX_QUEUE_SZ 4096        // The size of rx queue. Max is 4096.
#define TX_QUEUE_SZ 4096            // The size of tx queue. Max is 4096.
#define PKT_BURST_SZ 32            // Unfortunately it is 32 in DPDK 18
#define MAX_STR 256
#define MAX_INTERFACES 64
#define MAX_CORES 100

/* Output parameters */
#define VERBOSE -1
#define PRINT_STATS 1
#define INTERACTIVE_SHELL 1


#define DEBUG 0


#endif
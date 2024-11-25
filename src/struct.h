#ifndef STRUCT_H
#define STRUCT_H

typedef struct out_interface_sett
{
  int anon_enabled;
  int anon_mac_enabled;
  int anon_ip_enabled;
  char anon_ip_key_mode[MAX_STR];
  char anon_ip_key[MAX_STR];
  int anon_ip_rotation_delay;
  char anon_subnet_file[MAX_STR];
  char no_anon_subnet_file[MAX_STR];
  int engine;
  int anon_ext_ip;
  int dns;
  int tls;
  int http;
  int alpha;
  int delta;
} out_interface_sett;

typedef struct in_interface_sett
{
  int id;
  char address[MAX_STR];
  int n_out;
  int out_port[MAX_INTERFACES];
} in_interface_sett;

/* Types */

typedef struct ret_info
{
  char *offset;
  char *name;
  size_t strLen;
} ret_info;

typedef struct lru
{
  int full;
  int client_hash;
  time_t timestamp;
  struct lru *prev;
  struct lru *next;
} lru;

typedef struct client
{
  int active;
  int ipv;
  uint32_t ipv4_src;
  uint32_t ipv4_dst;
  __uint128_t ipv6_src;
  __uint128_t ipv6_dst;
  uint16_t in_port;
  uint16_t out_port;
  uint8_t protocol;
  time_t last_seen;
  struct lru *lru_ptr;
} client;

typedef struct names
{
  int full;
  char name[NAME_DNS];
  // char anon_name[100];
  int n_entry;
  int n_client;
  time_t oldest;
  struct names *prev;
  struct names *next;
  struct lru *head;
  struct lru *tail;
  client client_list[MAX_CLIENT];
} names;

typedef struct entry_access
{
  int number;
  pthread_mutex_t permission;
  // sem_t permission;
} entry_access;

typedef struct hash_struct
{
  entry_access *bitMap; //[FLOW_TABLE_SIZE];
  struct names *table;  //[FLOW_TABLE_SIZE];
} hash_struct;

typedef struct flow
{
  int ipv;
  uint32_t ipv4_src;
  uint32_t ipv4_dst;
  __uint128_t ipv6_src;
  __uint128_t ipv6_dst;
  uint16_t in_port;
  uint16_t out_port;
  uint8_t protocol;
  time_t timestamp;
} flow;

typedef struct table_flow
{
    int ipv;
    uint32_t ipv4_src;
    uint32_t ipv4_dst;
    uint32_t ipv4_anon;
    __uint128_t ipv6_src;
    __uint128_t ipv6_dst;
    uint8_t ipv6_anon[16];
    uint16_t in_port;
    uint16_t out_port;
    uint8_t  protocol;
    time_t timestamp;
    int toAnon;
    struct table_flow *prev;
    struct table_flow *next;
} table_flow;

typedef struct flow_mng
{
    struct table_flow *table;
    entry_access *bitMap;
} flow_mng;

#endif //STRUCT_H
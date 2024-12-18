#include "traffic_anon.h"

/* Reentrant data structure [core][interface] */
crypto_ip crypto_data[MAX_CORES][MAX_INTERFACES];
struct in_addr anon_net_list[MAX_SUBNETS];
struct in6_addr anon_net_listv6[MAX_SUBNETS];
int anon_net_mask[MAX_SUBNETS];
int anon_net_maskv6[MAX_SUBNETS];
int tot_anon_nets;
int tot_anon_netsv6;

struct in_addr no_anon_net_list[MAX_SUBNETS];
struct in6_addr no_anon_net_listv6[MAX_SUBNETS];
int no_anon_net_mask[MAX_SUBNETS];
int no_anon_net_maskv6[MAX_SUBNETS];
int tot_no_anon_nets;
int tot_no_anon_netsv6;

// sem_t mutex;

/* All data structure initialization must be here */
void process_packet_init(int nb_sys_core)
{
    int cnt = 0;
    for (int i = 0; i < MAX_INTERFACES; i++)
    {
        if (out_interface[i].anon_ip_enabled == 1)
        {
            if (strcmp(out_interface[i].anon_ip_key_mode, "static") == 0)
            {
                for (int j = 0; j < nb_sys_core; j++)
                    initialize_crypto(&crypto_data[j][i], out_interface[i].anon_ip_key, i, j);
            }
            FILE *fp = fopen(out_interface[i].anon_subnet_file, "r");
            ParseNetFile(fp,
                         "anonymized networks",
                         MAX_SUBNETS,
                         anon_net_list,
                         anon_net_listv6,
                         anon_net_mask,
                         anon_net_maskv6,
                         &tot_anon_nets,
                         &tot_anon_netsv6);

            fclose(fp);

            fp = fopen(out_interface[i].no_anon_subnet_file, "r");
            ParseNetFile(fp,
                         "anonymized networks",
                         MAX_SUBNETS,
                         no_anon_net_list,
                         no_anon_net_listv6,
                         no_anon_net_mask,
                         no_anon_net_maskv6,
                         &tot_no_anon_nets,
                         &tot_no_anon_netsv6);

            fclose(fp);
        }
    }
    proto_init(nb_sys_core);
}

/* Invoked per each packet */
void process_packet(struct rte_mbuf *packet, out_interface_sett interface_setting, int id, int core)
{
    int len;

    struct timespec tp, tp1;
    clockid_t clk_id = CLOCK_MONOTONIC_COARSE;
    clock_gettime(clk_id, &tp);
    len = rte_pktmbuf_data_len(packet);

    if (VERBOSE > 0)
        printf("ANON: Packet at core %d, len: %d\n", rte_lcore_id(), len);

    process_packet_eth(packet, interface_setting, tp);
    process_packet_ip(packet, interface_setting, id, core, tp);
}

/* Anonymize ethernet */
void process_packet_eth(struct rte_mbuf *packet, out_interface_sett interface_setting, struct timespec tp)
{
    int len;
    struct rte_ether_hdr *eth_hdr;

    len = rte_pktmbuf_data_len(packet);
    eth_hdr = rte_pktmbuf_mtod(packet, struct rte_ether_hdr *);

    packet->l2_len = sizeof(*eth_hdr);

    if (VERBOSE > 0)
    {
        len = rte_pktmbuf_data_len(packet);
        printf("ANON:    Len: %d\n", len);
        print_ether_addr("ANON:    eth src: ", &eth_hdr->src_addr);
        printf("\n");
        print_ether_addr("ANON:    eth dst: ", &eth_hdr->dst_addr);
        printf("\n");
    }

    if (interface_setting.anon_mac_enabled == 2)
    {

        // uint8_t *bytes = uint8_t[ETHER_ADDR_LEN];
        // bytes = (uint8_t) tp.tv_sec;

        eth_hdr->src_addr = eth_hdr->dst_addr = (struct rte_ether_addr){(uint8_t)tp.tv_sec, (uint8_t)tp.tv_sec, (uint8_t)tp.tv_sec, (uint8_t)tp.tv_sec, (uint8_t)tp.tv_sec, (uint8_t)tp.tv_sec};
        if (VERBOSE > 0)
            printf("ANON:    MAC addresses timestamped\n");
    }
    else if (interface_setting.anon_mac_enabled == 1)
    {
        eth_hdr->src_addr = eth_hdr->dst_addr = (struct rte_ether_addr){'\0', '\0', '\0', '\0', '\0', '\0'};
        if (VERBOSE > 0)
            printf("ANON:    MAC addresses removed\n");
    }
}

/* Anonymize IP */
void process_packet_ip(struct rte_mbuf *packet, out_interface_sett interface_setting, int id, int core, struct timespec tp)
{
    int len;
    uint16_t ether_type;
    char buf[MAX_STR];
    struct in_addr src_addr;
    struct in_addr dst_addr;
    struct in6_addr src_addr_6;
    struct in6_addr dst_addr_6;
    struct rte_ether_hdr *eth_hdr;
    struct rte_ipv4_hdr *ipv4_header;
    struct rte_ipv6_hdr *ipv6_header;
    // ip origin work as flag src-dst
    // 00 -> int-int
    // 11 -> ext-ext
    // 01 -> int-ext
    // 10 -> ext-int
    int ip_origin = 0;

    len = rte_pktmbuf_data_len(packet);
    eth_hdr = rte_pktmbuf_mtod(packet, struct rte_ether_hdr *);
    ether_type = htons(eth_hdr->ether_type);

    /* Is IPv4 */
    if (ether_type == 0x0800)
    {
        ipv4_header = rte_pktmbuf_mtod_offset(packet, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
        packet->l3_len = sizeof(*ipv4_header);

        src_addr.s_addr = ipv4_header->src_addr;
        dst_addr.s_addr = ipv4_header->dst_addr;

        if (VERBOSE > 0)
        {
            printf("ANON:    IPv4\n");
            printf("ANON:    from %s\n", inet_ntoa(src_addr));
            printf("ANON:    to   %s\n", inet_ntoa(dst_addr));
        }

        if (no_anon_ip_check(src_addr) || no_anon_ip_check(dst_addr))
        {
            // only anonymize those in anon_net_list
        }
        else
        {
            if (interface_setting.anon_ip_enabled == 1)
            {
                if (strcmp(interface_setting.anon_ip_key_mode, "static") == 0)
                {
                    if (anon_ip_check(src_addr))
                    {
                        src_addr.s_addr = retrieve_crypto_ip(&crypto_data[core][id], &src_addr, id, core);
                        ipv4_header->src_addr = src_addr.s_addr;
                    }
                    else
                    {
                        ip_origin += 10;
                    }
                    if (anon_ip_check(dst_addr))
                    {
                        dst_addr.s_addr = retrieve_crypto_ip(&crypto_data[core][id], &dst_addr, id, core);
                        ipv4_header->dst_addr = dst_addr.s_addr;
                    }
                    else
                    {
                        ip_origin += 1;
                    }

                    if (VERBOSE > 0)
                    {
                        printf("ANON:    new from %s\n", inet_ntoa(src_addr));
                        printf("ANON:    new to   %s\n", inet_ntoa(dst_addr));
                    }
                }
            }
            if (interface_setting.payload_drop_enabled == 1)
                l4_payload_remover(ipv4_header, NULL, packet, core, tp, id, interface_setting, &crypto_data[core][id], ip_origin);
            /* Apply K-anon */
            if (interface_setting.engine != 0)
                multiplexer_proto(ipv4_header, NULL, packet, core, tp, id, interface_setting, &crypto_data[core][id], ip_origin);
        }
    }
    /* Is IPv6 */
    else if (ether_type == 0x86DD)
    {

        ipv6_header = rte_pktmbuf_mtod_offset(packet, struct rte_ipv6_hdr *, sizeof(struct rte_ether_hdr));

        packet->l3_len = sizeof(*ipv6_header);

        // memcpy(&src_addr_6.s6_addr, ipv6_header->src_addr, sizeof(src_addr_6.s6_addr));
        // memcpy(&dst_addr_6.s6_addr, ipv6_header->dst_addr, sizeof(dst_addr_6.s6_addr));
        rte_memcpy(&src_addr_6.s6_addr, ipv6_header->src_addr, sizeof(src_addr_6.s6_addr));
        rte_memcpy(&dst_addr_6.s6_addr, ipv6_header->dst_addr, sizeof(dst_addr_6.s6_addr));

        if (VERBOSE > 0)
        {
            printf("ANON:    IPv6\n");
            inet_ntop(AF_INET6, &src_addr_6, buf, MAX_STR);
            printf("ANON:    from %s\n", buf);
            inet_ntop(AF_INET6, &dst_addr_6, buf, MAX_STR);
            printf("ANON:    to   %s\n", buf);
        }

        if (interface_setting.anon_ip_enabled == 1)
        {
            if (strcmp(interface_setting.anon_ip_key_mode, "static") == 0)
            {
                if (anon_ip_checkv6(src_addr_6))
                {
                    // sem_wait(&mutex);
                    src_addr_6 = *retrieve_crypto_ipv6(&crypto_data[core][id], &src_addr_6, id, core);
                    // memcpy(ipv6_header->src_addr, &src_addr_6.s6_addr, sizeof(src_addr_6.s6_addr));
                    rte_memcpy(ipv6_header->src_addr, &src_addr_6.s6_addr, sizeof(src_addr_6.s6_addr));
                    // sem_post(&mutex);
                }
                if (anon_ip_checkv6(dst_addr_6))
                {
                    // sem_wait(&mutex);
                    dst_addr_6 = *retrieve_crypto_ipv6(&crypto_data[core][id], &dst_addr_6, id, core);
                    // memcpy(ipv6_header->dst_addr, &dst_addr_6.s6_addr, sizeof(dst_addr_6.s6_addr));
                    rte_memcpy(ipv6_header->dst_addr, &dst_addr_6.s6_addr, sizeof(dst_addr_6.s6_addr));
                    // sem_post(&mutex);
                }

                // multiplexer_proto(NULL, ipv6_header, packet, core, tp, interface_setting.k_anon, interface_setting.k_delta);

                if (VERBOSE > 0)
                {
                    inet_ntop(AF_INET6, &src_addr_6, buf, MAX_STR);
                    printf("ANON:    new from %s\n", buf);
                    inet_ntop(AF_INET6, &dst_addr_6, buf, MAX_STR);
                    printf("ANON:    new to   %s\n", buf);
                }
            }
        }
        /* Apply K-anon */
        // NOT FULLY SUPPORTED YET
        // if(interface_setting.engine!=0)
        // multiplexer_proto(ipv4_header, NULL,  packet, core, tp, id, interface_setting.k_anon, interface_setting.k_delta, &crypto_data[core][id]);
    }
}

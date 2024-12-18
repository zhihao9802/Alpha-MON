#include <ldns/ldns.h>

#include <ldns/wire2host.h>
#include "traffic_anon.h"

hash_struct flow_db;

void proto_init(int nb_sys_cores)
{
    flow_db.bitMap = malloc(FLOW_TABLE_SIZE * sizeof(entry_access));
    if (flow_db.bitMap == NULL)
        return;
    flow_db.table = malloc(FLOW_TABLE_SIZE * sizeof(struct names));
    if (flow_db.table == NULL)
        return;
    table_init(&flow_db);
    flow_table_init();
}

void table_init(hash_struct *data)
{
    for (int i = 0; i < FLOW_TABLE_SIZE; i++)
    {
        data->bitMap[i].number = 0;
        // sem_init(&data->bitMap->permission, 0, 1);
        if (pthread_mutex_init(&data->bitMap[i].permission, NULL) != 0)
        {
            printf("\n mutex init failed\n");
            return;
        }
    }
}

// simple function to remove the all payload after TCP/UDP/ICMP header
void l4_payload_remover(struct rte_ipv4_hdr *ipv4_header, struct rte_ipv6_hdr *ipv6_header, struct rte_mbuf *packet, int core, struct timespec tp, int id, out_interface_sett interface_setting, crypto_ip *self, int ip_origin)
{
    struct rte_tcp_hdr *tcp_header;
    struct rte_udp_hdr *udp_header;
    flow newPacket;
    uint16_t src_port;
    uint16_t dst_port;
    int curr_hit;
    int flag;
    int detected_proto;
    struct table_flow *flusso = NULL;

    if (ipv4_header != NULL) // ipv4
    {
        switch (ipv4_header->next_proto_id)
        {
        case TCP:
            remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_tcp_hdr));
            break;
        case UDP:
            remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_udp_hdr));
            break;
        case ICMP:
            remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_icmp_hdr));
            break;
        default:
            /* Since the protocol is not defined, it is assumed not secure, so is deleted the IP payload*/
            remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr));
            break;
        }
    }
}

// For Protocol use:
// 0 = TCP
// 1 = UDP
void multiplexer_proto(struct rte_ipv4_hdr *ipv4_header, struct rte_ipv6_hdr *ipv6_header, struct rte_mbuf *packet, int core, struct timespec tp, int id, out_interface_sett interface_setting, crypto_ip *self, int ip_origin)
{
    struct rte_tcp_hdr *tcp_header;
    struct rte_udp_hdr *udp_header;
    flow newPacket;
    uint16_t src_port;
    uint16_t dst_port;
    int curr_hit;
    int flag;
    int detected_proto;
    struct table_flow *flusso = NULL;

    if (ipv4_header != NULL) // ipv4
    {
        switch (ipv4_header->next_proto_id)
        {
        case TCP:
            tcp_header = rte_pktmbuf_mtod_offset(packet, struct rte_tcp_hdr *, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr));
            packet->l4_len = sizeof(*tcp_header);
            newPacket.ipv = 4;
            newPacket.ipv4_src = ipv4_header->src_addr;
            newPacket.ipv4_dst = ipv4_header->dst_addr;
            newPacket.in_port = htons(tcp_header->src_port);
            newPacket.out_port = htons(tcp_header->dst_port);
            newPacket.protocol = ipv4_header->next_proto_id;
            newPacket.timestamp = tp.tv_sec;

            if (interface_setting.anon_ext_ip == 1)
            {
                if (ip_origin == 1 || ip_origin == 10)
                {
                    flusso = reference_flow(&newPacket);
                    if (flusso->toAnon == -1 || flusso->toAnon == 1)
                    {
                        flag = external_ip(packet, tp, ip_origin, flusso, &flow_db, newPacket, interface_setting.alpha, interface_setting.delta);
                        if (flag != 0)
                        {
                            flusso->toAnon = 1;
                        }
                        else
                        {
                            flusso->toAnon = 0;
                        }
                    }
                }
            }

            detected_proto = proto_detector(packet, 0, ipv4_header, NULL, newPacket.in_port, newPacket.out_port);

            if (detected_proto == 443)
            {
                if (interface_setting.tls != 0)
                {
                    flusso = reference_flow(&newPacket);
                    if (flusso->toAnon == -1 || flusso->toAnon == 1)
                    {
                        flag = tlsHelloEntry(packet, 0, ipv4_header, NULL, tcp_header->data_off, newPacket, &flow_db, interface_setting.alpha, interface_setting.delta, self, id, core);
                        if (flag != 0)
                            flusso->toAnon = 1;
                        else
                            flusso->toAnon = 0;
                    }
                }
                /*else
                    remove_payload(packet, sizeof(struct rte_ipv4_hdr)+sizeof(struct rte_ether_hdr)+sizeof(struct  rte_udp_hdr));*/
            }
            else if (detected_proto == 80)
            {
                if (interface_setting.http != 0)
                {
                    flusso = reference_flow(&newPacket);
                    if (flusso->toAnon == -1 || flusso->toAnon == 1)
                    {
                        flag = httpEntry(packet, 0, ipv4_header, NULL, tcp_header->data_off, newPacket, &flow_db, interface_setting.alpha, interface_setting.delta, self, id, core);
                        if (flag != 0)
                            flusso->toAnon = 1;
                        else
                            flusso->toAnon = 0;
                    }
                }
            }
            else if (newPacket.in_port != SSH && newPacket.in_port != HTTPS && newPacket.out_port != SSH && newPacket.out_port != HTTPS)
                remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_tcp_hdr));
            break;
        case UDP:
            udp_header = rte_pktmbuf_mtod_offset(packet, struct rte_udp_hdr *, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr));
            newPacket.ipv = 4;
            newPacket.ipv4_src = ipv4_header->src_addr;
            newPacket.ipv4_dst = ipv4_header->dst_addr;
            newPacket.in_port = htons(udp_header->src_port);
            newPacket.out_port = htons(udp_header->dst_port);
            newPacket.protocol = ipv4_header->next_proto_id;
            newPacket.timestamp = tp.tv_sec;

            if (interface_setting.anon_ext_ip == 1)
            {
                if (ip_origin == 1 || ip_origin == 10)
                {
                    flusso = reference_flow(&newPacket);
                    if (flusso->toAnon == -1 || flusso->toAnon == 1)
                    {
                        external_ip(packet, tp, ip_origin, flusso, &flow_db, newPacket, interface_setting.alpha, interface_setting.delta);
                        if (flag != 0)
                            flusso->toAnon == 1;
                        else
                            flusso->toAnon == 0;
                    }
                }
            }

            if (interface_setting.dns != 0)
            {
                if (proto_detector(packet, 1, ipv4_header, NULL, newPacket.in_port, newPacket.out_port) == DNS)
                {
                    dnsEntry(packet, 1, ipv4_header, NULL, newPacket, &flow_db, interface_setting.alpha, interface_setting.delta, self, id, core);
                }
                else
                    remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_udp_hdr));
            }
            else if (newPacket.in_port != SSH && newPacket.in_port != HTTPS && newPacket.out_port != SSH && newPacket.out_port != HTTPS)
                remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_udp_hdr));
            break;
        default:
            /* Since the protocol is not defined, it is assumed not secure, so is deleted the IP payload*/
            remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr));
            break;
        }
    }
    else if (ipv6_header != NULL) // ipv6 TO BE CORRECTED
    {
        switch (ipv6_header->proto)
        {
            /*case TCP:
                    tcp_header = rte_pktmbuf_mtod_offset(packet, struct rte_tcp_hdr *, sizeof(struct rte_ipv6_hdr)+sizeof(struct rte_ether_hdr) );
                    packet->l4_len = sizeof(*tcp_header);
                    newPacket.ipv = 6;
                    for( int temp=0; temp < 16; temp++)
                    {
                        newPacket.ipv6_src *= 10;
                        newPacket.ipv6_dst *= 10;
                        newPacket.ipv6_src += ipv6_header->src_addr[temp];
                        newPacket.ipv6_dst += ipv6_header->dst_addr[temp];
                    }
                    newPacket.in_port = htons(tcp_header->src_port);
                    newPacket.out_port = htons(tcp_header->dst_port);
                    newPacket.protocol = ipv6_header->proto;
                    newPacket.timestamp = tp.tv_sec;

                    if(interface_setting.tls!=0)
                    {
                        //TODO
                    }
                    else if(newPacket.in_port != SSH && newPacket.in_port != HTTPS  && newPacket.out_port != SSH && newPacket.out_port != HTTPS)
                        remove_payload(packet, sizeof(struct rte_ipv6_hdr)+sizeof(struct rte_ether_hdr)+sizeof(struct rte_tcp_hdr));
                    break;
            case UDP:
                    udp_header = rte_pktmbuf_mtod_offset(packet, struct  rte_udp_hdr *, sizeof(struct rte_ipv6_hdr)+sizeof(struct rte_ether_hdr) );
                    newPacket.ipv = 6;
                    for( int temp=0; temp < 16; temp++)
                    {
                            newPacket.ipv6_src *= 10;
                            newPacket.ipv6_dst *= 10;
                            newPacket.ipv6_src += ipv6_header->src_addr[temp];
                            newPacket.ipv6_dst += ipv6_header->dst_addr[temp];
                    }
                    newPacket.in_port = htons(udp_header->src_port);
                    newPacket.out_port = htons(udp_header->dst_port);
                    newPacket.protocol = ipv6_header->proto;
                    newPacket.timestamp = tp.tv_sec;

                    if(interface_setting.dns!=0)
                    {
                        if(newPacket.in_port == DNS || newPacket.out_port == DNS)
                        {
                            //TODO
                        }
                    }
                    else if(newPacket.in_port != SSH && newPacket.in_port != HTTPS  && newPacket.out_port != SSH && newPacket.out_port != HTTPS)
                        remove_payload(packet, sizeof(struct rte_ipv6_hdr)+sizeof(struct rte_ether_hdr)+sizeof(struct  rte_udp_hdr));
                    break;
            default:
                    remove_payload(packet, sizeof(struct rte_ipv6_hdr)+sizeof(struct rte_ether_hdr));
                    break;*/
        }
    }
}

void dnsEntry(struct rte_mbuf *packet, int protocol, struct rte_ipv4_hdr *ipv4_header, struct rte_ipv6_hdr *ipv6_header, flow newPacket, hash_struct *flow_db, int k_anon, int k_delta, crypto_ip *self, int id, int core)
{
    int stop, offset;
    dns_header *dns;
    struct res_record answers[100], auth[100], addit[100]; // the replies from the DNS server
    unsigned char *qname;
    char *buff = NULL;
    char *buffstart = NULL, *buffpay = NULL;
    char name[NAME_DNS];
    int dnslen;
    ret_info info;
    int ret;
    struct sockaddr_in a;
    size_t len;

    dnslen = packet->pkt_len;

    /* Retrieve DNS Header */
    dns = dns_header_extractor(packet, protocol, ipv4_header, ipv6_header);
    if (dns == NULL)
    {
        remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_udp_hdr));
        return;
    }

    if (ntohs(dns->qr) == 0) // DNS Question
    {
        if (DEBUG == 1)
            printf("It is DNS question\n");
        len = offset_extractor(protocol, ipv4_header, ipv6_header);
        if (len >= packet->pkt_len)
        {
            remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_udp_hdr));
            return;
        }
        buff = rte_pktmbuf_mtod_offset(packet, char *, offset_extractor(protocol, ipv4_header, ipv6_header));
        buffstart = buff;
        if (DEBUG == 1)
            printf("string: ");

        buff += 12; // Moving pointer to overcome header

        int i = 0;
        char c;

        int length = *buff++;
        if (length >= dnslen - (len + 12))
            return;
        else
            dnslen -= length;
        while (length != 0)
        {
            for (int j = 0; j < length; j++)
            {
                c = *buff++;
                name[i] = c;
                i++;
            }
            length = *buff++;
            if (length >= dnslen - (len + 12) || i >= NAME_DNS)
                return;
            else
                dnslen -= length;
            if (length != 0)
            {
                name[i] = '.';
                i++;
            }
        }
        name[i] = '\0';
        if (strlen(name) == 0)
            return;
        if (DEBUG == 1)
            printf("%s\n", name);

        // buff++;
        // unsigned short q = buff;
        // if(DEBUG==1)
        // printf("type: %d\n", ntohs(q));

        ret = table_add(flow_db, newPacket, name, k_anon, k_delta);
        if (DEBUG == 1)
            printf("Returned: %d\n", ret);
        // removing name
        if (ret < k_anon)
        {
            if (DEBUG == 1)
                printf("Removing: %s\n", name);
            buff = buffstart;
            buff += 12;
            length = *buff++;
            while (length != 0)
            {
                for (int j = 0; j < length; j++)
                {
                    *buff++ = randChar();
                }
                length = *buff++;
            }
        }
    }
    else if (ntohs(dns->qr) != 1) // DNS Response
    {
        if (DEBUG == 1)
            printf("It is DNS answer: ");
        if (DEBUG == 1)
            printf("Packet Condition: %d\n", (dns->rcode));

        // if((dns->rcode)==1 || (dns->rcode)==2)
        if (dns->rcode != 0)
        {
            if (DEBUG == 1)
                printf("Reply condition: %d\n", (dns->rcode));
            remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_udp_hdr));
            return;
        }
        if (DEBUG == 1)
            printf("%d Answers.\n", ntohs(dns->ans_count));
        int n_ans = ntohs(dns->ans_count);
        if (n_ans <= 0)
        {
            remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_udp_hdr));
            return;
        }
        int n_aut = ntohs(dns->auth_count);
        int n_add = ntohs(dns->add_count);
        len = offset_extractor(protocol, ipv4_header, ipv6_header);
        if (len >= packet->pkt_len)
        {
            remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_udp_hdr));
            return;
        }
        buffstart = buff = rte_pktmbuf_mtod_offset(packet, char *, offset_extractor(protocol, ipv4_header, ipv6_header));
        if (DEBUG == 1)
            printf("canversione char\n");
        if (DEBUG == 1)
            printf("string: ");

        buff += 12;

        // Removing name from query section
        int i = 0;
        char c;
        int length = *buff++;
        if (length >= dnslen - (len + 12))
            return;
        else
            dnslen -= length;
        while (length != 0)
        {
            for (int j = 0; j < length; j++)
            {
                c = *buff++;
                name[i] = c;
                i++;
            }
            length = *buff++;
            if (length >= dnslen - (len + 12) || i >= NAME_DNS)
                return;
            else
                dnslen -= length;
            if (length != 0)
            {
                name[i] = '.';
                i++;
            }
        }
        name[i] = '\0';
        if (strlen(name) == 0)
            return;
        if (DEBUG == 1)
            printf("%s\n", name);
        ret = table_add(flow_db, newPacket, name, k_anon, k_delta);
        if (DEBUG == 1)
            printf("Returned: %d\n", ret);
        if (ret < k_anon)
        {
            if (DEBUG == 1)
                printf("Removing: %s\n", name);
            buff = buffstart;
            buff += 12;
            length = *buff++;
            while (length != 0)
            {
                for (int j = 0; j < length; j++)
                {
                    *buff++ = randChar();
                }
                length = *buff++;
            }

            int stop = 0;
            if (DEBUG == 1)
                printf("devo ciclare %d volte\n", n_ans);
            len = len + 12 + strlen(name) + 2 * sizeof(unsigned short);
            if (len >= packet->pkt_len)
                return;
            buff += 2 * sizeof(unsigned short);

            for (i = 0; i < n_ans; i++)
            {
                //                answers[i].name=ReadName(buff,buffstart,&stop);
                if (ReadName(buff, buffstart, &stop) == 1)
                    return;
                if (DEBUG == 1)
                    printf("ANS:    Pars del nome\n");
                // printf("%s\n", answers[i].name);
                buff += stop;
                answers[i].resource = (r_data *)(buff);
                if (DEBUG == 1)
                    printf("ANS:    Pars r_data\n");
                // MOD for +2 bytes of typedef struct statement
                // buff += (sizeof(r_data)-2);
                len = len + (sizeof(unsigned short) + sizeof(unsigned short) + sizeof(unsigned int) + sizeof(unsigned short)) + ntohs(answers[i].resource->data_len);
                if (len >= packet->pkt_len)
                    return;
                buff += (sizeof(unsigned short) + sizeof(unsigned short) + sizeof(unsigned int) + sizeof(unsigned short));
                if (DEBUG == 1)
                    printf("ANS:    Pars r_data->type = %d\n", ntohs(answers[i].resource->type));
                if (ntohs(answers[i].resource->type) == 1) // if its an ipv4 address
                {
                    if (DEBUG == 1)
                        printf("ANS:    Pars ipv4\n");
                    //                    answers[i].rdata = (unsigned char*)malloc(ntohs(answers[i].resource->data_len));
                    for (int j = 0; j < ntohs(answers[i].resource->data_len); j++)
                    {
                        answers[i].rdata[j] = buff[j];
                    }

                    long *p;
                    struct in_addr addr, addr1;
                    p = (long *)answers[i].rdata;
                    addr1.s_addr = (*p); // working without ntohl
                    if (DEBUG == 1)
                        printf("has IPv4 address : %s\n", inet_ntoa(addr1));
                    addr.s_addr = retrieve_crypto_ip(self, &addr1, id, core);
                    if (DEBUG == 1)
                        printf("has IPv4 address : %s\n", inet_ntoa(addr));
                    (*p) = addr.s_addr;
                    // answers[i].rdata=(char*)p;
                    rte_memcpy(answers[i].rdata, p, sizeof(p));

                    for (int j = 0; j < ntohs(answers[i].resource->data_len); j++)
                    {
                        buff[j] = answers[i].rdata[j];
                    }

                    answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';
                    buff += ntohs(answers[i].resource->data_len);
                }
                else if (ntohs(answers[i].resource->type) == 5)
                {
                    if (DEBUG == 1)
                        printf("ANS:    Pars CNAME\n");
                    if (ReadName(buff, buffstart, &stop) == 1)
                        return;
                    len = len + ntohs(answers[i].resource->data_len);
                    if (len >= packet->pkt_len)
                        return;
                    buff += ntohs(answers[i].resource->data_len);
                }
                // TO BE IMPLEMENTED
                else if (ntohs(answers[i].resource->type) == 28)
                {
                    remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_udp_hdr) + 12);
                    return;
                    buff += ntohs(answers[i].resource->data_len);
                }
                else
                {
                    //                    answers[i].rdata = ReadName(buff,buffstart,&stop);
                    // TEST
                    // ReadName(buff,buffstart,&stop);
                    // buff += stop;
                    remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_udp_hdr) + 12);
                    return;
                }
            }
            // read authorities
            for (i = 0; i < n_aut; i++)
            {
                //                auth[i].name=ReadName(buff,buffstart,&stop);
                if (ReadName(buff, buffstart, &stop) == 1)
                    return;
                len = len + stop;
                if (len >= packet->pkt_len)
                    return;
                buff += stop;
                auth[i].resource = (r_data *)(buff);
                // MOD
                len = len + (sizeof(r_data) - 2);
                if (len >= packet->pkt_len)
                    return;
                buff += (sizeof(r_data) - 2);
                if (ntohs(auth[i].resource->type) == 2)
                {
                    //                auth[i].rdata=ReadName(buff,buffstart,&stop);
                    if (ReadName(buff, buffstart, &stop) == 1)
                        return;
                    len = len + stop;
                    if (len >= packet->pkt_len)
                        return;
                    buff += stop;
                }
                else
                {
                    len = len + ntohs(auth[i].resource->data_len);
                    if (len >= packet->pkt_len)
                        return;
                    buff += ntohs(auth[i].resource->data_len);
                }
            }
            // read additional
            for (i = 0; i < n_add; i++)
            {
                //                addit[i].name=ReadName(buff,buffstart,&stop);
                if (ReadName(buff, buffstart, &stop) == 1)
                    return;
                len = len + stop;
                if (len >= packet->pkt_len)
                    return;
                buff += stop;
                addit[i].resource = (r_data *)(buff);
                // MOD
                len = len + (sizeof(r_data) - 2);
                if (len >= packet->pkt_len)
                    return;
                buff += (sizeof(r_data) - 2);
                if (ntohs(addit[i].resource->type) == 1)
                {
                    //                    addit[i].rdata = (unsigned char*)malloc(ntohs(addit[i].resource->data_len));
                    len = len + ntohs(addit[i].resource->data_len);
                    if (len >= packet->pkt_len)
                        return;
                    for (int j = 0; j < ntohs(addit[i].resource->data_len); j++)
                        addit[i].rdata[j] = buff[j];

                    long *p;
                    struct in_addr addr, addr1;
                    p = (long *)addit[i].rdata;
                    addr1.s_addr = (*p); // working without ntohl
                    if (DEBUG == 1)
                        printf("has IPv4 address : %s\n", inet_ntoa(addr1));
                    addr.s_addr = retrieve_crypto_ip(self, &addr1, id, core);
                    if (DEBUG == 1)
                        printf("has IPv4 address : %s\n", inet_ntoa(addr));
                    (*p) = addr.s_addr;
                    // answers[i].rdata=(char*)p;
                    rte_memcpy(addit[i].rdata, p, sizeof(p));

                    for (int j = 0; j < ntohs(addit[i].resource->data_len); j++)
                        buff[j] = addit[i].rdata[j];

                    addit[i].rdata[ntohs(addit[i].resource->data_len)] = '\0';
                    buff += ntohs(addit[i].resource->data_len);
                }
                // TO BE IMPLEMENTED
                else if (ntohs(addit[i].resource->type) == 28)
                {
                    if (DEBUG == 1)
                        printf("ANS:    Skip ipv6\n");
                    remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_udp_hdr) + 12);
                    return;
                    buff += ntohs(addit[i].resource->data_len);
                }
                else
                {
                    //                    addit[i].rdata=ReadName(buff,buffstart,&stop);
                    // TEST
                    // ReadName(buff,buffstart,&stop);
                    // buff+=stop;
                    remove_payload(packet, sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_udp_hdr) + 12);
                    return;
                }
            }
        }
    }
}

void remove_dns_name(struct rte_mbuf *packet, ret_info info)
{
    for (int i = 0; i < info.strLen; i++)
    {
        info.offset = '0';
        info.offset++;
    }
}

void remove_payload(struct rte_mbuf *packet, size_t offset)
{
    char *buff;
    buff = rte_pktmbuf_mtod_offset(packet, char *, offset);
    for (int i = 0; i < (packet->pkt_len - offset); i++)
    {
        *buff = '\0';
        buff++;
    }
}

// int table_add(hash_struct * data, int hash, flow flow_recv)
int table_add(hash_struct *flow_db, flow flow_recv, char *name, int k_anon, int k_delta)
{
    int name_hash;
    int user_hash;
    struct names *curr_name = NULL;
    struct names *curr = NULL;
    int i, found, ret;
    // FILE *fp;
    // fp=fopen("test.csv", "a+");
    float probability;
    int seed;

    found = 0;
    name_hash = abs(nameHash(name));

    seed = (int)rte_get_tsc_cycles();
    seed = (214013 * seed + 2531011);
    probability = ((seed >> 16) & 0x7FFF) % 100;

    // sem_wait(&flow_db->bitMap->permission);
    pthread_mutex_lock(&flow_db->bitMap[name_hash].permission);

    // printf("name hash = %d\n", name_hash);
    curr = curr_name = &flow_db->table[name_hash];
    if (DEBUG == 1)
        printf("HASH name = %d\n", name_hash);
    if (flow_recv.ipv == 4)
    {
        user_hash = getHashClient(flow_recv.ipv4_src, MAX_CLIENT);
    }
    else if (flow_recv.ipv == 6)
    {
        user_hash = getHashClient(flow_recv.ipv6_src, MAX_CLIENT);
    }
    if (DEBUG == 1)
        printf("HASH user = %d\n", user_hash);

    if (probability < 33) // no clean
    {
        for (i = 0; i < flow_db->bitMap[name_hash].number; i++)
        {
            if (strcmp(curr_name->name, name) == 0)
            {
                found = 1;
                break;
            }
            else if (curr_name->next != NULL)
                curr_name = curr_name->next;
            else
                break;
        }
    }
    else if (33 <= probability <= 66) // partial clean
    {
        for (i = 0; i < flow_db->bitMap[name_hash].number; i++)
        {
            if (strcmp(curr_name->name, name) == 0)
            {
                // printf("Exit\n");
                found = 1;
                break;
            }
            else if (curr_name->next != NULL)
            {
                // printf("Next | ");
                if (i != 0)
                {
                    // printf("Not First | ");
                    if (curr_name->n_entry > 0)
                    {
                        // printf("Try Prune (%d)| ", curr_name->n_entry);
                        prune(curr_name, flow_recv.timestamp, k_delta);
                    }
                    // printf("Status (%d)| ", curr_name->n_entry);
                    if (curr_name->n_entry <= 0)
                    {
                        // printf("To Clean | ");
                        struct names *tmp = curr_name;
                        curr_name = curr_name->next;
                        // printf("New Pointer | ");
                        if (tmp->prev->prev != NULL)
                        {
                            // printf("Normal Case | ");
                            curr_name->prev->prev->next = curr_name;
                            curr_name->prev = curr_name->prev->prev;
                            // printf("Done | ");
                        }
                        else
                        {
                            // printf("First Case | ");
                            tmp->prev->next = curr_name;
                            curr_name->prev = tmp->prev;
                            // printf("Done | ");
                        }
                        flow_db->bitMap[name_hash].number--;
                        free(tmp);
                        // printf("Free | ");
                    }
                    else
                    {
                        // printf("No Clean | ");
                        curr_name = curr_name->next;
                    }
                }
                else
                {
                    // printf("First | ");
                    curr_name = curr_name->next;
                }
            }
            else
                break;
        }
    }
    else // complete clean
    {
        for (int j = 0; j < flow_db->bitMap[name_hash].number; j++)
        {
            if (strcmp(curr->name, name) == 0)
            {
                curr_name = curr;
                found = 1;
                i = j;
            }
            else if (curr->next != NULL)
            {
                if (i != 0)
                {
                    prune(curr, flow_recv.timestamp, k_delta);
                    if (curr->n_entry <= 0)
                    {
                        struct names *tmp = curr;
                        curr = curr->next;
                        if (tmp->prev->prev != NULL)
                        {
                            curr->prev->prev->next = curr;
                            curr->prev = curr->prev->prev; // da controllare: curr->prev = curr_name->prev->prev;
                        }
                        else
                        {
                            tmp->prev->next = curr;
                            curr->prev = tmp->prev;
                        }
                        flow_db->bitMap[name_hash].number--;
                        free(tmp);
                    }
                    else
                        curr = curr->next;
                }
                else
                    curr = curr->next;
            }
            else
                break;
        }
    }

    // fprintf(fp, "%d;%d\n", flow_db->bitMap[name_hash].number, i);
    // fclose(fp);
    if (found == 1) // entry gia' esiste: aggiorno
    {
        if (DEBUG == 1)
            printf("found\n");
        referencePage(curr_name, flow_recv, user_hash, k_anon, k_delta);
        ret = curr_name->n_entry;
    }
    else // devo creare nuova entry
    {
        if (DEBUG == 1)
            printf("new\n");
        if (flow_db->bitMap[name_hash].number == 0)
        {
            if (DEBUG == 1)
                printf("bitmap = %d\n", flow_db->bitMap[name_hash].number);
            flow_db->table[name_hash].full = 1;
            strcpy(flow_db->table[name_hash].name, name);

            flow_db->table[name_hash].oldest = flow_recv.timestamp;
            flow_db->table[name_hash].prev = NULL;
            flow_db->table[name_hash].next = NULL;
            flow_db->table[name_hash].n_entry = 0;

            if (DEBUG == 1)
                printf("parametri copiati: ");
            if (DEBUG == 1)
                printf("%d\n", flow_db->table[name_hash].client_list[user_hash].active);
            referencePage(&flow_db->table[name_hash], flow_recv, user_hash, k_anon, k_delta);

            flow_db->bitMap[name_hash].number++;
            ret = flow_db->table[name_hash].n_entry;
        }
        else
        {
            // struct names *newName = rte_malloc(NULL, sizeof(names), 0);
            struct names *newName = malloc(sizeof(names));

            if (newName != NULL)
            {

                curr_name->next = newName;
                newName->prev = curr_name;
                newName->next = NULL;
                newName->n_entry = 0;
                newName->n_client = 0;

                newName->head = NULL;
                newName->tail = NULL;

                newName->full = 1;
                strcpy(newName->name, name);
                newName->oldest = flow_recv.timestamp;

                for (int rescue = 0; rescue < MAX_CLIENT; rescue++)
                {
                    newName->client_list[rescue].active = 0;
                    newName->client_list[rescue].lru_ptr = NULL;
                }

                referencePage(newName, flow_recv, user_hash, k_anon, k_delta);

                flow_db->bitMap[name_hash].number++;
                ret = newName->n_entry;
            }
            else
                ret = 0;
        }
    }
    if (DEBUG == 1)
        printf(" Bitmap @%d = %d\n", name_hash, flow_db->bitMap[name_hash].number);

    // sem_post(&flow_db->bitMap->permission);
    pthread_mutex_unlock(&flow_db->bitMap[name_hash].permission);

    return ret;
}

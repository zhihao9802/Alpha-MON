#ifndef IP_UTILS_H
#define IP_UTILS_H

#define MAX_SUBNETS 256
#define BUF_SIZE 160//80

int anon_ip_check (struct in_addr adx);
int anon_ip_checkv6 (struct in6_addr adx);
int no_anon_ip_check (struct in_addr adx);
int no_anon_ip_checkv6 (struct in6_addr adx);
int match_ipv6_net(struct in6_addr adx, struct in6_addr *internal_list, int *mask_list, int list_size);

int ParseNetFile (  FILE *fp,
                    char *qualifier,
                    int max_entries, 
                    struct in_addr *CLASS_net_list,
                    struct in6_addr *CLASS_net_listv6,
                    int *CLASS_net_mask,
                    int *CLASS_net_mask_sizev6,
                    int *tot_CLASS_nets,
                    int *tot_CLASS_netsv6);

char * readline(FILE *fp, int skip_comment, int skip_void_lines);


#endif //IP_UTILS_H



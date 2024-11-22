//
//  http_mng.h
//  a-mon
//
//  Created by Thomas Favale on 06/07/2020.
//  Copyright © 2020 Thomas Favale. All rights reserved.
//

#ifndef http_mng_h
#define http_mng_h

char * http_header_extractor (struct rte_mbuf *, int, struct rte_ipv4_hdr *, struct rte_ipv6_hdr *);
size_t offset_extractor_http (int, struct rte_ipv4_hdr *, struct rte_ipv6_hdr *, uint8_t);
int httpEntry (struct rte_mbuf *, int, struct rte_ipv4_hdr *, struct rte_ipv6_hdr *, uint8_t, flow, hash_struct *, int, int, crypto_ip *, int, int);

#endif /* http_mng_h */

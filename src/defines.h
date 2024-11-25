#ifndef DEFINES_H
#define DEFINES_H

/* Protocol Type */
#define TCP             0x06
#define UDP    	        0x11
#define ICMP            0x01

/* Protocol Port */
#define FTP_DATA	20
#define FTP_CONTROL 	21
#define SSH 		22
#define HTTP 		80
#define HTTPS 		443
#define DNS 		53

#define MAX_CLIENT 900
#define FLOW_TABLE_SIZE 10000//900
#define NAME_DNS 500

/* Protocol Type */
#define T_OUT             300
#define TCP_UDP_FLOWS     2000003

#endif //DEFINES_H
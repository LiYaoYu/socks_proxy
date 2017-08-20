#ifndef _SOCKS4_H
#define _SOCKS4_H

#include <netinet/in.h>

#define MAXBUFFSIZE 16383
#define MAXUSERID 15
#define MAXDOMNAME 63
//#define LISTENFTP "37214"

#define REJECT 0
#define GRANT 1
#define BIND 2

typedef struct socks4_packet {
    unsigned char vn;
    unsigned char cd;
    int dst_port;
    char dst_ip[INET_ADDRSTRLEN + 1];
    char uid[MAXUSERID + 1];
    //char domName[MAXDOMNAME + 1];
} socks4_packet;

void run_conn_mode(socks4_packet* s4ptr, int srcfd);

void run_bind_mode(socks4_packet* s4ptr, int srcfd);

void get_socks_info(int clsockfd, socks4_packet* s4packet);

void send_socks4_ctrl_pkt(int clsockfd, socks4_packet* s4packet, int status);

void forward_data(int s4client, int dsthost);

void show_socks_info(socks4_packet* s4packet);

int parse_socks_port(unsigned char* c_input);

void parse_socks_ip(unsigned char* c_input, char* ip);

void parse_socks_uid(unsigned char* c_input, char* id);

//void getDomName(char* c_input, char* domain);

int ispassed_fw(char* ip);

#endif

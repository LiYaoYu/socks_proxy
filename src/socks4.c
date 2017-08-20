#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>

#include "socks4.h"
#include "tcp.h"

void run_conn_mode(socks4_packet* s4ptr, int srcfd)
{
    int dstfd;
    int connStat;

    // connect to dst
    dstfd = active_tcp(s4ptr->dst_ip, s4ptr->dst_port);
    connStat = dstfd != CONN_FAIL? GRANT : REJECT;

    // send reply
    send_socks4_ctrl_pkt(srcfd, s4ptr, connStat);

    if (connStat == GRANT) {
        forward_data(srcfd, dstfd);
        close(srcfd);
        close(dstfd);
        exit(EXIT_SUCCESS);
    } else {
        close(srcfd);
        puts("Connect to dst IP failed");
        exit(EXIT_FAILURE);
    }
}

void run_bind_mode(socks4_packet* s4ptr, int srcfd)
{
    int dstfd;
    int connStat;
    char listenftp[6];
    extern int listenfd;

    srand(getpid());

    snprintf(listenftp, 6, "%d", 37210 + rand() % 3721);

    passive_tcp(listenftp);

    connStat = listenfd != -1? BIND : REJECT;

    // send bind mode reply
    send_socks4_ctrl_pkt(srcfd, s4ptr, connStat);

    if (connStat == BIND) {
        // accept fd from destination
        dstfd = accept_client();

        // send sock pkt again to tell s4 client ftp server is accepted
        send_socks4_ctrl_pkt(srcfd, s4ptr, connStat);

        forward_data(srcfd, dstfd);
        close(srcfd);
        close(dstfd);
        exit(EXIT_SUCCESS);
    } else {
        close(srcfd);
        puts("Connect to dst IP failed");
        exit(EXIT_FAILURE);
    }
}

void get_socks_info(int clsockfd, socks4_packet* s4packet)
{
    int tmp;
    int n_read = 0;
    unsigned char c_input[MAXBUFFSIZE + 1];
    while ((tmp = read(clsockfd, &c_input[n_read], sizeof(c_input))) > 0) {
        n_read += tmp;
        if (c_input[n_read - 1] == 0)
            break;
    }

    // parse socks4 packet
    s4packet->vn = c_input[0];
    s4packet->cd = c_input[1];
    s4packet->dst_port = parse_socks_port(c_input);
    parse_socks_ip(c_input, s4packet->dst_ip);
    parse_socks_uid(c_input, s4packet->uid);
    // getDomName(c_input, s4packet->domName);
}

void send_socks4_ctrl_pkt(int clsockfd, socks4_packet* s4packet, int status)
{
    // socks 4 packet size is 8 bytes
    unsigned char buffer[8];

    int i;

    buffer[0] = 0;
    buffer[1] = status == GRANT || status == BIND? 90 : 91;

    if (status == BIND) {
        extern struct sockaddr_in svaddr; // needed when accept client
        unsigned short int bindedport = ntohs(svaddr.sin_port);

        // get port
        buffer[2] = bindedport / 256;
        buffer[3] = bindedport % 256;

        // get IP
        for (i = 4; i < 8; i++)
            buffer[i] = 0;
    } else {
        // get port
        buffer[2] = s4packet->dst_port / 256;
        buffer[3] = s4packet->dst_port % 256;

        // get IP
        buffer[4] = atoi(strtok(s4packet->dst_ip, "."));

        for (i = 5; i < 8; i++)
            buffer[i] = atoi(strtok(NULL, "."));
    }

    // print status
    if (status == BIND)
        puts("SOCKS_BIND GRANTED");
    else if (status == GRANT)
        puts("SOCKS_CONNECT GRANTED");
    else
        puts("SOCKS_CONNECT REJECTED");

    // send reply packet
    if (write(clsockfd, buffer, 8) < 8)
        perror("write in send_socks4_ctrl_pkt");
}

void forward_data(int s4client, int dsthost)
{
    puts("dual forward...");

    int nfds;
    int n_read;
    char buffer[MAXBUFFSIZE + 1];
    fd_set afds;
    fd_set rfds;

    // initialization
    nfds = s4client >= dsthost? s4client + 1 : dsthost + 1;
    FD_ZERO(&afds);
    FD_SET(s4client, &afds);
    FD_SET(dsthost, &afds);

    // forwarding
    while (1) {
        memcpy(&rfds, &afds, sizeof(afds));

        select(nfds, &rfds, NULL, NULL, NULL);

        // handle two directional forwarding
        if (FD_ISSET(s4client, &rfds)) { // sent to dsthost
            // read from s4 client with nonblock
            n_read = recv(s4client, buffer, sizeof(buffer), MSG_DONTWAIT);

            // read nothing without blocking
            if (n_read <= 0 && errno != EAGAIN )
                break;

            // send buffer to dsthost
            /*
            if (send(dsthost, buffer, n_read, MSG_DONTWAIT) < 0\
                    && errno == EAGAIN) {
            	perror("write to dst host in forward_data");
            }
            */
            if (write(dsthost, buffer, n_read) < 0)
                perror("write to dst host in forward_data");

            // refresh buffer
            memset(buffer, 0, sizeof(buffer));
        }
        if (FD_ISSET(dsthost, &rfds)) { // sent to s4client
            // read from dst host with nonblock
            n_read = recv(dsthost, buffer, sizeof(buffer), MSG_DONTWAIT);

            // read nothing without blocking
            if (n_read <= 0 && errno != EAGAIN )
                break;

            // send buffer to s4client
            /*
            if (send(s4client, buffer, n_read, MSG_DONTWAIT) < 0\
                    && errno == EAGAIN) {
            	perror("write to s4client in forward_data");
            }
            */
            if (write(s4client, buffer, n_read) < 0)
                perror("write to s4client in forward_data");

            // refresh buffer
            memset(buffer, 0, sizeof(buffer));
        }
    }
}

void show_socks_info(socks4_packet* ptr)
{
    extern char c_ip[INET_ADDRSTRLEN];
    extern struct sockaddr_in claddr; // need to show port
    printf("SRC IP: %s, SRC PORT: %d\n", c_ip, ntohs(claddr.sin_port));
    printf("DST IP: %s, DST PORT: %d\n", ptr->dst_ip, ptr->dst_port);
    printf("VN: %d, CD: %d, USERID: %s\n", ptr->vn, ptr->cd, ptr->uid);
}

int parse_socks_port(unsigned char* c_input)
{
    int port;

    // c_input[2] & c_input[3] is the dst port field
    port = c_input[2] * 256 + c_input[3];
    return port;
}

void parse_socks_ip(unsigned char* c_input, char* ip)
{
    int stat;
    stat = snprintf(ip, INET_ADDRSTRLEN + 1,\
                    "%u.%u.%u.%u",c_input[4], c_input[5], c_input[6], c_input[7]);

    if (stat == -1)
        perror("parse_socks_ip");
}

void parse_socks_uid(unsigned char* c_input, char* id)
{
    int stat;
    stat = snprintf(id, MAXUSERID + 1, "%s", c_input + 8);

    if (stat == -1)
        perror("parse_socks_uid");
}

/*
void getDomName(char* c_input, char* domain)
{
	// how to get this? and is this needed?
	strtok(c_input, "\0");
	domain = strtok(NULL, "\0");
}
*/

// check whether the ip can pass the firewall or not
int ispassed_fw(char* ip)
{
    char* reach;
    char buf[INET_ADDRSTRLEN + 1];

    FILE* fptr;
    fptr = fopen("firewall.conf", "r");

    // compare ip to the passed ones in firewall.conf
    while (fgets(buf, sizeof(buf), fptr)) {
        strtok(buf, "\r\n");

        // handle ip like 140.113.*.* or *.*.*.*
        reach = strchr(buf, '*');
        *reach = 0;

        if (reach == buf || strncmp(buf, ip, strlen(buf)) == 0)
            return 1;
    }
    return 0;
}

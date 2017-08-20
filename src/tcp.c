#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "tcp.h"

int listenfd;
char c_ip[INET_ADDRSTRLEN]; // need to show IP
struct sockaddr_in svaddr; // needed when accept client
struct sockaddr_in claddr; // need to show port

// wait for browser and destination
void passive_tcp(char* port)
{
    // set sockaddr_in struct
    svaddr.sin_family = AF_INET;
    svaddr.sin_port = htons(atoi(port));
    svaddr.sin_addr.s_addr = INADDR_ANY;

    // create TCP connection socket
    if ((listenfd = socket(svaddr.sin_family, SOCK_STREAM, 0)) == -1) {
        perror("socket\n");
        // exit(EXIT_FAILURE);
    }
    int optval = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
        perror("setsockopt");
    }
    if (bind(listenfd, (struct sockaddr*) &svaddr, sizeof(svaddr)) == -1) {
        perror("bind\n");
        // exit(EXIT_FAILURE);
    }
    if (listen(listenfd, BACKLOG) == -1) {
        perror("listen\n");
        exit(EXIT_FAILURE);
    } else
        puts("wait for connection...");
}

// accept from browser and destination
int accept_client()
{
    int connfd;
    socklen_t addrLen = sizeof(struct sockaddr_in);

    // wait for connections
    connfd = accept(listenfd, (struct sockaddr*)&claddr, &addrLen);
    if (connfd == -1) {
        perror("accept\n");
        exit(EXIT_FAILURE);
    } else {
        // get the text form of IP addr. from binary
        inet_ntop(svaddr.sin_family, &claddr.sin_addr.s_addr,\
                  c_ip, sizeof(c_ip));

        printf("Connected from %s\nport: %d\n",\
               c_ip, ntohs(claddr.sin_port));
    }
    return connfd;
}

// active connect to browser and destination
int active_tcp(char* ip, int port)
{
    int connfd;
    struct sockaddr_in actaddr;

    // set sockaddr_in struct
    actaddr.sin_family = AF_INET;
    actaddr.sin_port = htons(port);
    actaddr.sin_addr.s_addr = inet_addr(ip);

    // create TCP connection socket
    if ((connfd = socket(actaddr.sin_family, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // connect to server
    if (connect(connfd, (struct sockaddr*)&actaddr, sizeof(actaddr)) == -1) {
        perror("connect in active_tcp");
        return CONN_FAIL;
    }
    return connfd;
}

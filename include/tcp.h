#ifndef _TCP_H
#define _TCP_H

#include "socks4.h"

#define BACKLOG 10
#define CONN_FAIL 0

void passive_tcp(char* portNumber);

int accept_client();

int active_tcp(char* ip, int port);

#endif

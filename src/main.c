#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include "tcp.h"
#include "socks4.h"

int main(int argc, char* argv[])
{
    // check usage
    if (argc != 2) {
        puts("Usage: ./server <Port Number>");
        exit(EXIT_FAILURE);
    }

    // initial connection
    passive_tcp(argv[1]);

    while (1) {
        int connfd = accept_client();
        int pid = fork();

        if (pid == -1) { // failed fork
            perror("fork");
            close(connfd);
            puts("wait for connection...");
            continue;
        } else if (pid > 0) { // parent
            close(connfd);
            continue;
        } else { // child
            socks4_packet *s4ptr = malloc(sizeof(socks4_packet));

            get_socks_info(connfd, s4ptr); // get s4ptr info
            show_socks_info(s4ptr); // print the packet info
            fflush(stdout);

            // TODO: handle the ip = 0.0.0.x by domain name

            // check pass or not
            if (!ispassed_fw(s4ptr->dst_ip)) { // cannot pass fire wall
                // send reply
                send_socks4_ctrl_pkt(connfd, s4ptr, REJECT);
                exit(EXIT_SUCCESS);
            }

            if (s4ptr->cd == 1) // connect mode
                run_conn_mode(s4ptr, connfd);
            else // bind mode
                run_bind_mode(s4ptr, connfd);
        }

    }
}

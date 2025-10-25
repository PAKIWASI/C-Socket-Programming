#include "socket_includes.h"
#include <stdlib.h>


int main(void)
{
    int listenfd;
    int connfd;
    size_t n;
    struct sockaddr_in servaddr;
    u8 sendmsg[BUFFER_SIZE + 1];
    u8 recvmsg[BUFFER_SIZE + 1];


    // create a socket (which is like a file, but data goes to client instead of disk)
//  file discriptor        ipv4             tcp             ip protocol
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        err_n_die("socket error");
    }

     // Set socket option to reuse address 
    int optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // server responds to anything
    servaddr.sin_port = htons(SERVER_PORT); // port to listen to


    // bind socket to the address/port
    if ( (bind(listenfd, (SA*)&servaddr, sizeof(servaddr))) < 0) {
        err_n_die("bind error");
    }

    // listen for 10 req
    if ( (listen(listenfd, 10)) < 0) {
        err_n_die("listen error");
    }

    printf("Server listening on port %d\n", SERVER_PORT);

    while (1) {
        struct sockaddr_in addr; // stores info about client (each client gets a socket)
        socklen_t addr_len = sizeof(addr);

        // accespt blocks until n incoming connection arrives
        // it returns a file descriptor to the connection
        printf("Waiting for a connection on port %d\n", SERVER_PORT);
        fflush(stdout);

        //Await a connection on socket listenfd.it returns When a connection arrives, open a new socket to communicate with it,
        connfd = accept(listenfd, (SA*)&addr, &addr_len); // the null vals can have the address of the client (not needed rn)
        if (connfd < 0) {
            perror("accept error");
            continue;
        }

        //Print client IP address
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Connection to client: %s:%d\n", client_ip, ntohs(addr.sin_port));


        // msg exchange loop
        while (1) {

            memset(recvmsg, 0, BUFFER_SIZE);

            // recienve msg
            n = recv(connfd, recvmsg, BUFFER_SIZE - 1, 0);
            if (n < 0) {
                perror("recv error");
                break;
            } else if (n == 0) {
                printf("Client disconnected\n");
                break;
            }

            recvmsg[n] = '\0';

            // print the recienved msg
            printf("Client says: %s\n", recvmsg);
            char* hex = bin2hex(recvmsg, n);
            printf("Hex dump: %s\n", hex);
            free(hex);

            // prepare response
            snprintf((char*)sendmsg, sizeof(sendmsg), "Message recieved (%zu bytes)", n);

            if ( send(connfd, sendmsg, strlen((char*)sendmsg), 0) < 0) {
                perror("send error");
                break;
            }


        }

        // close temp connection with client
        close(connfd);
        printf("Connection closed\n");
    }
}




#include "socket_includes.h"


int main(int argc, char* argv[])
{
    int sockfd;   // socket file discriptor
    ssize_t n;   
    
    // all info needed to identify a network endpoint(ip address, port number)
    struct sockaddr_in servaddr;  // stores addressing info for internal sockets
    char sendmsg[BUFFER_SIZE];
    char recvmsg[BUFFER_SIZE];

    if (argc != 2) {
        err_n_die("usage: %s <server address>", argv[0]);
    }

//  file discriptor        ipv4             tcp             ip protocol
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        err_n_die("Error while creating socket");
    }

    memset(&servaddr, 0, sizeof(servaddr)); // initilising servaddr's bytes to 0 for padding 
    servaddr.sin_family = AF_INET;                      // address family (always AF_INET for ipv4)
    servaddr.sin_port = htons(SERVER_PORT);  // port number in network byte order (which is hex in reverse)
        
        // internet presentation to network (converts human readable ip addresses to binary), we pass a string arg
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) { // the bytes stores in sin_addr (ipv4 address)
        err_n_die("inet_pton error for %s ", argv[1]);  
    }

        // Open a connection on socket to peer at servaddr
    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) < 0) {
        err_n_die("connection failed!");
    }

    printf("Connected to server at %s:%d\n", argv[1], SERVER_PORT);
    printf("Type messages to send (type 'quit' to exit):\n\n");

    // message exchange loop
    while (1) {
        printf("You: ");
        fflush(stdout);

        // user input (stored in sendmsg)
        if ( fgets(sendmsg, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        // Remove newline if present
        size_t len = strlen(sendmsg);
        if (len > 0 && sendmsg[len-1] == '\n') {
            sendmsg[len-1] = '\0';
            len--;
        }

        if ( strcmp(sendmsg, "quit") == 0) {
            printf("Closing connection...\n");
            break;
        }


        // send the msg
        ssize_t total_sent = 0;
        while (total_sent < (ssize_t)len) {
            ssize_t sent = send(sockfd, sendmsg + total_sent, len - total_sent, 0);
            if (sent < 0) { err_n_die("send error"); }
            total_sent += sent;
        }

        // receive the response
        memset(recvmsg, 0, BUFFER_SIZE);
        n = recv(sockfd, recvmsg, BUFFER_SIZE - 1, 0);
        if (n < 0) {
            err_n_die("recv error");
        } else if (n == 0) {
            printf("Server closed connection\n");
            break;
        }

        recvmsg[n] = '\0';
        printf("Server: %s\n\n", recvmsg);
    }

    close(sockfd);
    return 0;
}


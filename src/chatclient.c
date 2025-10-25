#include "socket_includes.h"
#include <sys/select.h>



int main(int argc, char* argv[]) 
{
    int sockfd;                     // Socket file descriptor for server connection
    struct sockaddr_in servaddr;    // Structure to store server address information
    char buffer[BUFFER_SIZE];       // Buffer for sending/receiving messages
    fd_set readfds;                 // File descriptor set for select() monitoring    
    
    if (argc != 2) {
        err_n_die("usage: %s <server address>", argv[0]);
    }
    
//  file discriptor        ipv4             tcp             ip protocol
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        err_n_die("Error while creating socket");
    }
    
    // Setup server address
    memset(&servaddr, 0, sizeof(servaddr)); // set to 0
    servaddr.sin_family = AF_INET;                   // ipv4 
    servaddr.sin_port = htons(SERVER_PORT);    // Set server port in network byte order
    
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        err_n_die("inet_pton error for %s", argv[1]);
    }
    
    // Connect to server
    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) < 0) {
        err_n_die("connection failed!");
    }
    
    printf("Connected to chat server at %s:%d\n", argv[1], SERVER_PORT);
    printf("Type /help for commands, or just type to chat!\n");
    printf("Type 'quit' to exit\n\n");
    
    // Main loop - monitor both server and user input
    while (1) {

        FD_ZERO(&readfds);              // Clear all bits in the readfds set
        FD_SET(sockfd, &readfds);       // Add server socket to monitoring set
        FD_SET(STDIN_FILENO, &readfds); // Add stdin (keyboard input) to monitoring set        
        
        int maxfd = sockfd;  // Highest file descriptor number for select()
        // sockfd is always > STDIN_FILENO (0), so it's the maximum
        
        // Wait for activity on either socket or stdin
        // - maxfd + 1: highest fd to check + 1 (select() requirement)
        // - &readfds: monitor these fds for read readiness
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            err_n_die("select error");
        }
        
        // Check if server sent data
        if (FD_ISSET(sockfd, &readfds)) {
            // server has sent data for us, we need to receive it
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            
            if (n <= 0) { // network error or server disconnected us
                if (n < 0) {
                    perror("recv error");
                } else {
                    printf("\nServer closed connection\n");
                }
                break;
            }
            
            // successfully received the data
            buffer[n] = '\0';           // null terminate it
            printf("%s", buffer);
            fflush(stdout);
        }
        
        // Check if user typed something
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            // User pressed Enter - read their input
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
                break;
            }
            
            // Remove trailing newline
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n') {
                buffer[len-1] = '\0';
                len--;
            }
            
            // Check for quit command
            if (strcmp(buffer, "quit") == 0) {
                printf("Disconnecting...\n");
                break;
            }
            
            // Send message to server
            if (len > 0) {
                // Add newline back for server processing
                buffer[len] = '\n';
                buffer[len + 1] = '\0';
                
                if (send(sockfd, buffer, len + 1, 0) < 0) {
                    err_n_die("send error");
                }
            }
        }
    }
    
    close(sockfd);
    return 0;
}

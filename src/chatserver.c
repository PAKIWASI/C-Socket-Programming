#include "socket_includes.h"

#include <sys/select.h>
#include <strings.h>



#define MAX_CLIENTS 100
#define USERNAME_MAX 20


typedef struct {        // Client Info
    int sockfd;
    char username[USERNAME_MAX + 1];
    int active;
} Client;


Client clients[MAX_CLIENTS];        // array of clients


void init_clients(void) {                       
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sockfd = -1;
        clients[i].username[0] = '\0';
        clients[i].active = 0;
    }
}

int add_client(int sockfd) {                // add a client (add their file descriptor)
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].sockfd = sockfd;
            clients[i].active = 1;                              // username is User + fd
            snprintf(clients[i].username, USERNAME_MAX + 1, "User%d", clients[i].sockfd);
            return i;
        }
    }
    return -1;
}

void remove_client(int index) {
    if (index >= 0 && index < MAX_CLIENTS) {
        close(clients[index].sockfd);
        clients[index].sockfd = -1;
        clients[index].username[0] = '\0';
        clients[index].active = 0;
    }
}

Client* find_client_by_username(const char* username) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcasecmp(clients[i].username, username) == 0) {
            return &clients[i];
        }
    }
    return NULL;
}

void broadcast_message(const char* msg, int sender_index) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && i != sender_index) {
            send(clients[i].sockfd, msg, strlen(msg), 0);
        }
    }
}

void send_to_client(int index, const char* msg) {
    if (index >= 0 && index < MAX_CLIENTS && clients[index].active) {
        send(clients[index].sockfd, msg, strlen(msg), 0);
    }
}

void send_user_list(int client_index) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "\n=== Online Users ===\n");
    send_to_client(client_index, buffer);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            snprintf(buffer, sizeof(buffer), "[%s] %s\n", 
                    clients[i].username,
                    (i == client_index) ? " (you)" : "");
            send_to_client(client_index, buffer);
        }
    }
    snprintf(buffer, sizeof(buffer), "====================\n");
    send_to_client(client_index, buffer);
}

void handle_command(int client_index, char* msg) 
{
    char buffer[BUFFER_SIZE];
    
    // Remove trailing newline/whitespace
    size_t len = strlen(msg);
    while (len > 0 && (msg[len-1] == '\n' || msg[len-1] == '\r' || msg[len-1] == ' ')) {
        msg[--len] = '\0';
    }
    
    if (strncmp(msg, "/name ", 6) == 0) {
        char* new_name = msg + 6;       // rest of the message after the command
        while (*new_name == ' ') { new_name++; } 
        
        if (strlen(new_name) == 0 || strlen(new_name) > USERNAME_MAX) {
            send_to_client(client_index, "Invalid username length\n");
            return;
        }
        
        if (find_client_by_username(new_name) != NULL) {
            send_to_client(client_index, "Username already taken\n");
            return;
        }
        
        snprintf(buffer, sizeof(buffer), "*** %s is now known as %s ***\n", 
                clients[client_index].username, new_name);
        broadcast_message(buffer, -1);
        
        strncpy(clients[client_index].username, new_name, USERNAME_MAX);
        clients[client_index].username[USERNAME_MAX] = '\0';
        
        send_to_client(client_index, "Username changed successfully\n");
    }
    else if (strncmp(msg, "/msg ", 5) == 0) {
        char* rest = msg + 5;                   // rest of the message after the command + space (the USERNAME) 
        char* space = strchr(rest, ' '); // space after the username (before msg starts)
        
        if (space == NULL) {
            send_to_client(client_index, "Usage: /msg <username> <message>\n");
            return;
        }
        
        *space = '\0';
        char* target_username = rest;
        char* message = space + 1; // this points to message 
        
        Client* target = find_client_by_username(target_username);
        if (target == NULL) {
            send_to_client(client_index, "User not found\n");
            return;
        }
        
        snprintf(buffer, sizeof(buffer), "[PM from %s]: %s\n", 
                clients[client_index].username, message);
        send(target->sockfd, buffer, strlen(buffer), 0);
        
        snprintf(buffer, sizeof(buffer), "[PM to %s]: %s\n", 
                target->username, message);

        send_to_client(client_index, buffer);
    }
    else if (strcmp(msg, "/list") == 0) {
        send_user_list(client_index);
    }
    else if (strcmp(msg, "/help") == 0) {
        const char* help = 
            "\n=== Commands ===\n"
            "/name <username>  - Change your username\n"
            "/msg <user> <msg> - Send private message\n"
            "/list             - List online users\n"
            "/help             - Show this help\n"
            "================\n";
        send_to_client(client_index, help);
    }
    else {
        send_to_client(client_index, "Unknown command. Type /help for commands\n");
    }
}



int main(void) 
{
    int listenfd;                  // main server socket (file discriptor) that listens to incomming connections
    int connfd;                    // per-client socket for actual communication
    int maxfd;                     // tracks  the highest file discriptor number for select() efficiency     
    struct sockaddr_in servaddr;   // stores server's address info (IP, port, etc.)    
    struct sockaddr_in clientaddr; // stores client's address info when they connect
    socklen_t clientlen;           // Size of client address structure (needed for accept())  
    fd_set readfds;                // Temporary file descriptor set for select() (select modifies)
    fd_set master_fds;             // Master set that tracks ALL active sockets
    char buffer[BUFFER_SIZE];      // Buffer for sending/receiving messages
    

    init_clients();
    

    // create a socket (which is like a file, but data goes to client instead of disk)
//  file discriptor        ipv4             tcp             ip protocol
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        err_n_die("socket error");
    }
    
    int optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    // server address setup
    memset(&servaddr, 0, sizeof(servaddr));        // init to 0
    servaddr.sin_family = AF_INET;                          // ipv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // server responds to anything
    servaddr.sin_port = htons(SERVER_PORT);      // port to listen to
    
    // bind socket to address
    if (bind(listenfd, (SA*)&servaddr, sizeof(servaddr)) < 0) {
        err_n_die("bind error");
    }
    
    // start listenig to req on the port
    if (listen(listenfd, 10) < 0) {     // 10 = backlog size (max queued connections)
        err_n_die("listen error");
    }
    
    printf("Chat server listening on port %d\n", SERVER_PORT);
    printf("Max clients: %d\n\n", MAX_CLIENTS);
    
    // SELECT() SETUP
    FD_ZERO(&master_fds);           // Clear all bits in master_fds (initialize to empty)
    FD_SET(listenfd, &master_fds);  // Add listening socket to master set
    maxfd = listenfd;               // Currently, listening socket is the highest fd
    


    // MAIN SERVER LOOP - runs forever
    while (1) {
        readfds = master_fds; // Copy master set because select() MODIFIES the set
        
        // CALL SELECT() - waits for activity on any socket                 // NULL timeout = wait forever until activity
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            err_n_die("select error");
        }
        
        // Check for new connections (listening socket activity)
        if (FD_ISSET(listenfd, &readfds)) {
            // New connection waiting - accept it
            clientlen = sizeof(clientaddr);
            // accept() creates a NEW socket for this specific client
            connfd = accept(listenfd, (SA*)&clientaddr, &clientlen);
            
            if (connfd < 0) {
                perror("accept error");
                continue;
            }

            // ADD CLIENT TO MANAGEMENT SYSTEM
            int client_index = add_client(connfd); // connfd points to the socket of the client
            if (client_index == -1) { 
                const char* msg = "Server full. Try again later.\n";
                send(connfd, msg, strlen(msg), 0);
                close(connfd);
                printf("Connection refused: server full\n");
            } else {
                FD_SET(connfd, &master_fds); // Add client socket to select() monitoring
                if (connfd > maxfd) {        // Update maxfd if this is new highest
                    maxfd = connfd;
                }
                
                char client_ip[INET_ADDRSTRLEN]; // log the successfull connection
                inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip, sizeof(client_ip));
                printf("New connection: %s [ID: %d, Username: %s]\n", 
                       client_ip, clients[client_index].sockfd, clients[client_index].username);
                
                // notify user
                snprintf(buffer, sizeof(buffer), 
                        "Welcome to the chat! Your username is: %s\nType /help for commands\n",
                        clients[client_index].username);
                send_to_client(client_index, buffer);
                
                // broadcast to entire chat
                snprintf(buffer, sizeof(buffer), "*** %s has joined the chat ***\n", 
                        clients[client_index].username);
                broadcast_message(buffer, client_index);
            }
        }
        
        // Check existing clients for incomming messages
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) { continue; }
            
            int sockfd = clients[i].sockfd;     // get client's sockfd that wants to send a msg
            if (FD_ISSET(sockfd, &readfds)) {   // This client has sent data - receive it
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
                
                if (n <= 0) {    // client disconnected or error
                    if (n < 0) { // error
                        perror("recv error");
                    }
                    printf("Client disconnected: %s [ID: %d]\n", 
                           clients[i].username, clients[i].sockfd);
                    
                    // broadcast the exit
                    snprintf(buffer, sizeof(buffer), "*** %s has left the chat ***\n", 
                            clients[i].username);
                    broadcast_message(buffer, i);
                    
                    // cleanup
                    FD_CLR(sockfd, &master_fds);  // Remove from select() monitoring
                    remove_client(i);      // Remove from client management system
                } else {        // successfully received data !
                    buffer[n] = '\0';
                    
                    if (buffer[0] == '/') { // message is a commangd
                        handle_command(i, buffer);
                    } else {            // broadcast message
                        size_t len = strlen(buffer);
                        while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) {
                            buffer[--len] = '\0'; // Remove trailing newline
                        }
                        
                        if (len > 0) {  // broadcast the message
                            char msg[BUFFER_SIZE];
                            snprintf(msg, sizeof(msg), "[%s]: %s\n", 
                                    clients[i].username, buffer);
                            printf("%s", msg); // also log to server console
                            broadcast_message(msg, i);
                        }
                    }
                }
            }
        }
    }
    
    close(listenfd);
    return 0;
}

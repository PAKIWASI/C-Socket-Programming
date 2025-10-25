#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>


#define SERVER_PORT 6969 

#define BUFFER_SIZE 4096  // size of the buffer to store response 

#define SA struct sockaddr
#define u8 uint8_t

// exit on error function
void err_n_die(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(1);
}

char* bin2hex(const unsigned char* input, size_t len) {
    if (input == NULL || len <= 0) { return NULL; }
    
    const char* hexits = "0123456789ABCDEF";
    char* result = malloc((len * 3) + 1);
    if (!result) { return NULL; }
    
    for (size_t i = 0; i < len; i++) {
        result[i * 3] = hexits[input[i] >> 4];
        result[(i * 3) + 1] = hexits[input[i] & 0x0F];
        result[(i * 3) + 2] = ' ';
    }
    
    if (len > 0) {
        result[(len * 3) - 1] = '\0';
    } else {
        result[0] = '\0';
    }
    
    return result;
}


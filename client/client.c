#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

#include <regex.h>

#include "../lib/tw_packet.h"

// CLIENT
// ./twmailer-client <ip> <port> 

int main(int argc, char *argv[]) {

    // Check input
    if (argc < 3) {
        printf("Invalid arguments. Usage: ./twmailer-client <ip> <port>\n");
        exit(1);
    }

    // Check and convert domains etc to IPv4 addresses
    struct addrinfo requirements;
    memset(&requirements, 0, sizeof(requirements));

    requirements.ai_family = AF_INET;
    requirements.ai_socktype = SOCK_DGRAM;
    requirements.ai_flags = 0;
    requirements.ai_protocol = 0;


    struct addrinfo *server;
    char* ip = argv[1];
    char* port = argv[2];
    int errno = getaddrinfo(ip, port, &requirements, &server);

    if(errno != 0) {
        printf("An error occured. Getaddrinfo Error: %s\n", gai_strerror(errno));
        exit(1);
    }

    // Try to connect to given IP address
    int socketID;
    struct addrinfo *results;
    for( results = server; results != NULL; results = server->ai_next ) {
        socketID = socket(results->ai_family, results->ai_socktype, results->ai_protocol);

        if(socketID == -1)
            continue;

        if(connect(socketID, results->ai_addr, results->ai_addrlen) == -1) {
            printf("Connection could not be established...\n");
            exit(1);
        } else break;

        close(socketID);
    }

    freeaddrinfo(server);

    if(results == NULL) {
        printf("Could not connect.\n");
        exit(1);
    }

    // Client Hello
    if(send(socketID, "hello", 6 * sizeof(char), MSG_NOSIGNAL) == -1) {
        printf("Connection could not be established...\n");
        exit(1);
    }

    // Server Hello
    char buffer[100];
    if(read(socketID, buffer, 100) == -1) {
        printf("Connection could not be established...\n");
        exit(1);
    }

    // Connected
    printf("Successfully connected to server. Please enter a command.\n");
    int running = 1;
    while(running) {
        char* input = readline("Please enter a command: ");
        
        PACKET_TYPE type = str2type(input);

        TW_PACKET answer;
        answer.header = INVALID;

        switch (type)
        {
            case SEND:
                answer = TW_PACKET_IO(socketID, type, -1, "Receiver: ", "Subject: ", "Message:\n", NULL); break;
            case LIST:
                answer = TW_PACKET_IO(socketID, type, 0, NULL); break;
            case READ:
                answer = TW_PACKET_IO(socketID, type, 1, "Index: "); break;
            case DELETE:
                answer = TW_PACKET_IO(socketID, type, 1, "Index: "); break;
            case LOGIN:
                answer = TW_PACKET_IO(socketID, type, 2, "Username: ", "Password: "); break;
            case QUIT: TW_PACKET_IO(socketID, QUIT, 0, NULL); running = 0; break;
            default: 
                break;
        }

        if(answer.header != INVALID) if(type != LIST) print_TW_PACKET(&answer); else print_TW_PACKET_INDEXED(&answer); 

        free(input);
    }

    close(socketID);
    return 0;
}
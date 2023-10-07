#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

#include <regex.h>
#include <readline/readline.h>

#include "../lib/tw_packet.h"

void TW_SEND(int);
void TW_LIST(int);
void TW_READ(int);
void TW_DEL(int);

int checkUsername(char*);

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
    while(1) {
        char* input = readline("Please enter a command: ");
        PACKET_TYPE type = str2type(input);

        switch(type) {
            case SEND: TW_SEND(socketID); break;
            case LIST: TW_LIST(socketID); break;
            case READ: TW_READ(socketID); break;
            case DELETE: TW_DEL(socketID); break;
            case LOGIN: break; // TODO
            default: break;
        }

        free(input);
    }

    return 0;
}

int checkUsername(char* username) {
    regex_t regex;

    if(regcomp(&regex, "^([a-z0-9]){1,8}$", 0) != 0) {
        printf("Regex Error.");
        exit(1);
    }

    int ret = regexec(&regex, username, 0, NULL, 0);

    if(!ret) {
        return 1;
    } else if(ret == REG_NOMATCH) {
        return 0;
    } else {
        printf("Regex Compare Error.");
        exit(1);
    }
}

void TW_SEND(int socketID) {
    char *message = get_input(-1);
    TW_PACKET packet;

    printf("Message:\n\n%s", message);

    packet = make_TW_PACKET(SEND, message);
    send_TW_PACKET(socketID, &packet);

    free(message);

    TW_PACKET received = receive_TW_PACKET(socketID);
    print_TW_PACKET(&received);
}

void TW_LIST(int socketID) {
    char *message = get_input(1);
    TW_PACKET packet;

    packet = make_TW_PACKET(LIST, message);
    send_TW_PACKET(socketID, &packet);

    TW_PACKET received = receive_TW_PACKET(socketID);
    print_TW_PACKET(&received); 
}

void TW_READ(int socketID) {
    char *message = get_input(2);
    TW_PACKET packet;

    packet = make_TW_PACKET(READ, message);
    send_TW_PACKET(socketID, &packet);

    TW_PACKET received = receive_TW_PACKET(socketID);
    print_TW_PACKET(&received); 
}

void TW_DEL(int socketID) {
    char *message = get_input(2);
    TW_PACKET packet;

    packet = make_TW_PACKET(DELETE, message);
    send_TW_PACKET(socketID, &packet);

    TW_PACKET received = receive_TW_PACKET(socketID);
    print_TW_PACKET(&received); 
}
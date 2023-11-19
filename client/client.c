#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>

#include <regex.h>

#include "../lib/tw_packet.h"

int running = 1;

// CLIENT
// ./twmailer-client <ip> <port> 

void signalHandler(int sig);

int main(int argc, char *argv[]) {

    // Check input
    if (argc < 3) {
        printf("Invalid arguments. Usage: ./twmailer-client <ip> <port>\n");
        exit(1);
    }

    // Register signal handler
    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        perror("signal can not be registered");
        return EXIT_FAILURE;
    }

    // Create socket, connect and receive server hello
    int sockfd;
    char buffer[1024];
    struct sockaddr_in address;
    int message_size;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket error");
        return EXIT_FAILURE;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;       
    address.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &address.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("Connect error - no server available");
        return EXIT_FAILURE;
    }
    printf("Connection with server (%s) established\n", inet_ntoa(address.sin_addr));

    message_size = recv(sockfd, buffer, 1023, 0);
    if (message_size == -1) {
        perror("recv error");
    } else if (message_size == 0) {
        printf("Server closed remote socket\n"); 
        running = 0;
    } else {
        buffer[message_size] = '\0';
        printf("%s", buffer);
    }

    int logged_in = 0;
    while(running) {    
        // Collect input commnad
        char* input = readline("Please enter a command: ");
        if(!running) {
            free(input);
            break;   
        }

        PACKET_TYPE type = str2type(input);
        TW_PACKET answer = {
            .header.type = INVALID,
            .header.size = 0
        };
        
        if(!logged_in && type != LOGIN && type != QUIT) {
            printf("You have to log in first! (Command LOGIN)\n");
            free(input);
            continue;
        }

        // Depending on command, build the corresponding packets, send them and receive a response
        switch (type) {
            case SEND:
                answer = TW_PACKET_IO(sockfd, type, -1, "Receiver: ", "Subject: ", "Message:\n", NULL); break;
            case LIST:
                answer = TW_PACKET_IO(sockfd, type, 0, NULL); break;
            case READ:
                answer = TW_PACKET_IO(sockfd, type, 1, "Index: "); break;
            case DELETE:
                answer = TW_PACKET_IO(sockfd, type, 1, "Index: "); break;
            case LOGIN:
                answer = TW_PACKET_IO(sockfd, type, 2, "Username: ", "Password: ");
                logged_in = answer.header.type == SERVER_OK ? 1 : 0;
                break;
            case QUIT: TW_PACKET_IO(sockfd, QUIT, 0, NULL); running = 0; break;
            default: 
                break;
        }

        // Print the response
        if(answer.header.type != INVALID) {
            if(type != LIST) print_TW_PACKET(&answer); 
            else print_TW_PACKET_INDEXED(&answer);
        }

        if(answer.header.type == QUIT) {
            running = 0;
        }

        free(input);
        if(answer.header.size != 0) free_TW_PACKET(&answer);
    }

    // On shutdown, close socket
    if (sockfd != -1)
    {
        if (shutdown(sockfd, SHUT_RDWR) == -1) {
            perror("shutdown create_socket"); 
        } if (close(sockfd) == -1) {
            perror("close create_socket");
        }
        sockfd = -1;
    }
    return 0;
}

void signalHandler(int sig) {
    if (sig == SIGINT) {
        printf("\nAbort Requested... (press enter)\n");
        running = 0;
    }
    else
    {
        exit(sig);
    }
}
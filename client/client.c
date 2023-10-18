#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <regex.h>

#include "../lib/tw_packet.h"

#define PORT 10101

// CLIENT
// ./twmailer-client <ip> <port> 

int main(int argc, char *argv[]) {

    // Check input
    if (argc < 3) {
        printf("Invalid arguments. Usage: ./twmailer-client <ip> <port>\n");
        exit(1);
    }

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
    address.sin_port = htons(PORT);
    if (argc < 2) {
        inet_aton("127.0.0.1", &address.sin_addr);
    } else {
        inet_aton(argv[1], &address.sin_addr);
    }

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
    } else {
        buffer[message_size] = '\0';
        printf("%s", buffer);
    }

    int running = 1;
    while(running) {
        char* input = readline("Please enter a command: ");
        
        PACKET_TYPE type = str2type(input);

        TW_PACKET answer;
        answer.header = INVALID;

        switch (type)
        {
            case SEND:
                answer = TW_PACKET_IO(sockfd, type, -1, "Receiver: ", "Subject: ", "Message:\n", NULL); break;
            case LIST:
                answer = TW_PACKET_IO(sockfd, type, 0, NULL); break;
            case READ:
                answer = TW_PACKET_IO(sockfd, type, 1, "Index: "); break;
            case DELETE:
                answer = TW_PACKET_IO(sockfd, type, 1, "Index: "); break;
            case LOGIN:
                answer = TW_PACKET_IO(sockfd, type, 2, "Username: ", "Password: "); break;
            case QUIT: TW_PACKET_IO(sockfd, QUIT, 0, NULL); running = 0; break;
            default: 
                break;
        }

        if(answer.header != INVALID) {
            if(type != LIST) print_TW_PACKET(&answer); 
            else print_TW_PACKET_INDEXED(&answer);
        } 

        free(input);
    }

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
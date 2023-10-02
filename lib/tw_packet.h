#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

#include <readline/readline.h>

#define MAX_MESSAGE_SIZE 1000

// PACKETS
typedef enum PACKET_TYPE {
    LOGIN,
    SEND,
    LIST,
    READ,
    DELETE,
    SERVER_OK,
    SERVER_ERR
} PACKET_TYPE;

typedef struct TW_PACKET {
    PACKET_TYPE header;
    char data[MAX_MESSAGE_SIZE];
} TW_PACKET;

void send_TW_PACKET(int sockfd, TW_PACKET *packet);
TW_PACKET receive_TW_PACKET(int sockfd);
TW_PACKET make_TW_PACKET(PACKET_TYPE type, char *message);
void print_TW_PACKET(TW_PACKET *packet);
char* get_input(int lines);
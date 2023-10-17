#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

#include "tw_utility.h"

#define MAX_MESSAGE_SIZE 1000

// PACKETS
typedef enum PACKET_TYPE {
    LOGIN,
    SEND,
    LIST,
    READ,
    DELETE,
    SERVER_OK,
    SERVER_ERR,
    QUIT,
    INVALID
} PACKET_TYPE;

typedef struct TW_PACKET {
    PACKET_TYPE header;
    char data[MAX_MESSAGE_SIZE];
} TW_PACKET;

TW_PACKET TW_PACKET_IO(int sockfd, PACKET_TYPE type, int lines, ...);
void send_TW_PACKET(int sockfd, TW_PACKET *packet);
TW_PACKET receive_TW_PACKET(int sockfd);
TW_PACKET make_TW_PACKET(PACKET_TYPE type, int lines, va_list *prompts);
TW_PACKET make_TW_SERVER_PACKET(PACKET_TYPE type, char* payload);
void print_TW_PACKET(TW_PACKET *packet);
void print_TW_PACKET_INDEXED(TW_PACKET *packet);
char* get_input(int lines, va_list *prompts);
const char* type2str(PACKET_TYPE type);
PACKET_TYPE str2type(char* str);
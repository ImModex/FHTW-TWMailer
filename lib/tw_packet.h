#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>

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
    SERVER_ERR,
    INVALID
} PACKET_TYPE;

typedef struct TW_PACKET {
    PACKET_TYPE header;
    char data[MAX_MESSAGE_SIZE];
} TW_PACKET;

TW_PACKET handle_TW_PACKET(int sockfd, PACKET_TYPE type);
TW_PACKET TW_PACKET_IO(int sockfd, PACKET_TYPE type, int lines, ...);
void send_TW_PACKET(int sockfd, TW_PACKET *packet);
TW_PACKET receive_TW_PACKET(int sockfd);
TW_PACKET make_TW_PACKET(PACKET_TYPE type, int lines, va_list *prompts);
void print_TW_PACKET(TW_PACKET *packet);
char* get_input(int lines, va_list *prompts);
PACKET_TYPE str2type(char* str);
#include "tw_packet.h"

// Handles making, sending and receiving an answer packet
TW_PACKET TW_PACKET_IO(int sockfd, PACKET_TYPE type, int lines, ...) {
    va_list prompts;
    va_start(prompts, lines);

    TW_PACKET packet = make_TW_PACKET(type, lines, &prompts);
    va_end(prompts);

    send_TW_PACKET(sockfd, &packet);
    free_TW_PACKET(&packet);

    TW_PACKET received;
    if(type != QUIT) received = receive_TW_PACKET(sockfd);

    return received;
}

// Handles sending of a packet
void send_TW_PACKET(int sockfd, TW_PACKET *packet) {
    if(write(sockfd, &packet->header, sizeof(packet->header)) != sizeof(packet->header)) {
        printf("Could not send message header to server!\n");
        exit(1);
    }

    if(write(sockfd, packet->data, packet->header.size) != packet->header.size) {
        printf("Could not send message data to server!\n");
        exit(1);
    }
}

// Handles receiving of a packet
TW_PACKET receive_TW_PACKET(int sockfd) {
    TW_PACKET packet;
    int message_size = 0;
    if((message_size = read(sockfd, &packet.header, sizeof(TW_PACKET_HEADER))) == -1) {
        printf("Connection could not be established...\n");
        exit(1);
    }

    if(message_size == 0) {
        packet.header.type = QUIT;
        packet.header.size = 0;
        packet.data = NULL;
        return packet;
    }

    packet.data = calloc(packet.header.size, sizeof(char));

    if((message_size = read(sockfd, packet.data, packet.header.size)) == -1) {
        printf("Connection could not be established...\n");
        exit(1);
    }

    if(message_size == 0) {
        packet.header.type = QUIT;
        packet.header.size = 0;
        packet.data = NULL;
        return packet;
    }

    return packet;
}

// Creates a server-type packet
TW_PACKET make_TW_SERVER_PACKET(PACKET_TYPE type, char* payload) {
    if(payload == NULL) return make_TW_PACKET(type, 0, NULL);

    TW_PACKET packet;
    	
    packet.data = NULL;
    packet.header.type = type;
    tw_strcpy(&packet.data, type2str(type));
    tw_strcat(&packet.data, payload);
    packet.header.size = strlen(packet.data) + 1;

    return packet;
}

// Creates a general packet
TW_PACKET make_TW_PACKET(PACKET_TYPE type, int lines, va_list *prompts) {
    TW_PACKET packet;
    packet.header.type = type;
    packet.data = NULL;

    tw_strcpy(&packet.data, type2str(type));
    packet.header.size = strlen(packet.data) + 1;

    if(type != SERVER_OK && type != SERVER_ERR && lines != 0) {
        char *message = get_input(lines, prompts);
        tw_strcat(&packet.data, message);
        packet.header.size = strlen(packet.data) + 1;
        free(message);
    }
        
    return packet;
}

// Prints a packet
void print_TW_PACKET(TW_PACKET *packet) {
    if((*packet).header.type == INVALID) {
        perror("Invalid packet.");
        return;
    }

    printf("%s\n", packet->data);
}

// Prints a packet with indexes (used for LIST)
void print_TW_PACKET_INDEXED(TW_PACKET *packet) {
    if((*packet).header.type == INVALID) {
        perror("Invalid packet.");
        return;
    }

    char** split = split_data(packet->data);

    int i = 0;
    while(split[i] != NULL) {
        if(!i) printf("%s\n", split[i]); else printf("%d: %s\n", i-1, split[i]);
        ++i;
    }

    printf("\n");
    free_data(&split);
}

// Frees a packet
void free_TW_PACKET(TW_PACKET *packet) {
    if(packet->data != NULL) free(packet->data);
    packet->data = NULL;
}

// Gets variable length of input with variable length of prompts
char* get_input(int lines, va_list *prompts) {
    if(lines == 0) return NULL;

    int var_end = 0;
    int lines_read = 0;
    char *buf, *message = (char*) malloc(sizeof(char));
    memset(message, 0, sizeof(char));

    do {
        if(prompts != NULL) {
            char* prompt = va_arg(*prompts, char*);
            if(prompt == NULL) var_end = 1;
            if(!var_end) printf("%s", prompt);
            fflush(stdout);
        }
        buf = readline("");

        if(lines == -1 && lines_read >= 2 && strcmp(buf, ".") == 0) {
            free(buf);
            break;
        }

        message = (char*) realloc(message, strlen(message) + strlen(buf) + 2);
        if(message == NULL) {
            perror("realloc failed in get_input");
            exit(1);
        }
        sprintf(message, "%s%s\n", message, buf);

        free(buf);
        if(++lines_read >= lines && lines != -1) break;
    } while(1);

    return message;
}

// Convert string to packet type
PACKET_TYPE str2type(char* str) {
    if(strcmp(str, "SEND") == 0) {
        return SEND;
    } else if(strcmp(str, "LIST") == 0) {
        return LIST;
    } else if(strcmp(str, "READ") == 0) {
        return READ;
    } else if(strcmp(str, "DEL") == 0) {
        return DELETE;
    } else if(strcmp(str, "LOGIN") == 0) {
        return LOGIN;
    } else if(strcmp(str, "OK") == 0) {
        return SERVER_OK;
    } else if(strcmp(str, "ERR") == 0) {
        return SERVER_ERR;
    } else if(strcmp(str, "QUIT") == 0) {
        return QUIT;
    }

    return INVALID;
}

// Convert packet type to string
const char* type2str(PACKET_TYPE type) {
    switch (type)
    {
        case SERVER_OK: return "OK\n";
        case SERVER_ERR: return "ERR\n";
        case LIST: return "LIST\n";
        case READ: return "READ\n";
        case DELETE: return "DEL\n";
        case LOGIN: return "LOGIN\n";
        case QUIT: return "QUIT\n";
        default: return "";
    }
}
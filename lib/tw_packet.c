#include "tw_packet.h"

void send_TW_PACKET(int sockfd, TW_PACKET *packet) {
    if(write(sockfd, packet, sizeof(*packet)) != sizeof(*packet)) {
        printf("Could not send message to server!\n");
        exit(1);
    }
}

TW_PACKET receive_TW_PACKET(int sockfd) {
    TW_PACKET packet;
    if(read(sockfd, &packet, sizeof(TW_PACKET)) == -1) {
        printf("Connection could not be established...\n");
        exit(1);
    }

    return packet;
}

const char* apply_header(PACKET_TYPE type) {
    switch (type)
    {
        case LIST: return "LIST\n";
        case READ: return "READ\n";
        case DELETE: return "DEL\n";
        default: return "";
    }
}

TW_PACKET make_TW_PACKET(PACKET_TYPE type, char *message) {
    TW_PACKET packet;

    packet.header = type;
    strcpy(packet.data, apply_header(type));
    strcat(packet.data, message);

    return packet;
}

void print_TW_PACKET(TW_PACKET *packet) {
    printf("%s\n", packet->data);
}

char* get_input(int lines) {
    if(lines == 0) return NULL;

    int lines_read = 0;
    char *buf, *message = (char*) malloc(sizeof(char));
    memset(message, 0, sizeof(char));

    do {
        buf = readline("");

        if(strcmp(buf, ".") == 0) break;

        message = (char*) realloc(message, strlen(message) + strlen(buf) + 2);
        if(message == NULL) {
            perror("realloc failed in get_input");
            exit(1);
        }
        sprintf(message, "%s%s\n", message, buf);

        if(lines != -1 && ++lines_read >= lines) break;
    } while(strcmp(buf, ".") != 0);

    return message;
}

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
    }

    return INVALID;
}
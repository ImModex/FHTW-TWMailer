#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "../lib/tw_packet.h"
#include "../lib/tw_utility.h"

#define MAIL_DIR "./inbox"
// ./inbox/<user>/date_sender

typedef struct session {
    int logged_in;
    char* username;
} session;

void init();
char** split_data(char* data);
void free_data(char*** split);
TW_PACKET login(char* username, char* password);
TW_PACKET sv_send(session* session, char* content);
TW_PACKET list(session* session);
TW_PACKET sv_read(session* session, int index);
TW_PACKET del(session* session, int index);

unsigned long hash(unsigned char *str);

int main(int argc, char *argv[])
{
    init();

    int                      sfd, s;
    ssize_t                  nread;
    socklen_t                peer_addrlen;
    struct addrinfo          hints;
    struct addrinfo          *result, *rp;
    struct sockaddr_storage  peer_addr;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);

    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(NULL, argv[1], &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);

    }

    /* getaddrinfo() returns a list of address structures.
     *               Try each address until we successfully bind(2).
     *                             If socket(2) (or bind(2)) fails, we (close the socket
     *                                           and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;                  /* Success */

        close(sfd);

    }

    freeaddrinfo(result);           /* No longer needed */

    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }

    TW_PACKET pktBuf;
    /* Read datagrams and echo them back to sender. */
    for (;;) {
        memset(&pktBuf, 0, sizeof(pktBuf));
        char host[NI_MAXHOST], service[NI_MAXSERV];

        peer_addrlen = sizeof(peer_addr);
        nread = recvfrom(sfd, &pktBuf, sizeof(pktBuf), 0,
                (struct sockaddr *) &peer_addr, &peer_addrlen);
        if (nread == -1)
            continue;

        if(nread == 6) {
            sendto(sfd, "hello", 6, 0, (struct sockaddr *) &peer_addr, peer_addrlen);
            continue;
        }

        s = getnameinfo((struct sockaddr *) &peer_addr,
                peer_addrlen, host, NI_MAXHOST,
                service, NI_MAXSERV, NI_NUMERICSERV);
        if (s == 0) {
            printf("Received %zd bytes from %s:%s\n", nread, host, service);
            print_TW_PACKET(&pktBuf);

            switch(pktBuf.header) {
                case SEND: sv_send(NULL, pktBuf.data); break;
                default: break;
            }

            TW_PACKET ans = make_TW_SERVER_PACKET(SERVER_OK);
            sendto(sfd, &ans, sizeof(ans), 0, (struct sockaddr *) &peer_addr, peer_addrlen);
        }
        else
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
    }
}

void init() {
    if(access(MAIL_DIR, F_OK) != 0)
        mkdir(MAIL_DIR, 0700);
}

char** split_data(char* data) {
    char** split = (char**) malloc(sizeof(char*));

    if(split == NULL) {
        perror("Memory Error (split_data)");
        exit(1);
    }

    int index = 0;
    char* prevPtr = data;
    char* ptr = data;
    while(*ptr != '\0') {
        while(*(ptr++) != '\n');

        split[index] = malloc((ptr - prevPtr) * sizeof(char));
        strncpy(split[index], prevPtr, (ptr - prevPtr) - sizeof(char));

        prevPtr = ptr;
        ++index;

        split = realloc(split, (index + 1) * sizeof(char*));
        split[index] = NULL;
    }

    return split;
}

void free_data(char*** split) {
    char** _split = *split;
    int index = 0;

    while(_split[index] != NULL) {
        free(_split[index++]);
    }

    free(*split);
}

// SEND

// 0 SENDER
// 1 RECEIVER
// 2 SUBJ
// 3 MESSAGE
TW_PACKET sv_send(session* session, char* content) {
    char **split = split_data(content);
    char path[1024];
    // ./inbox/<user>/date_sender

    sprintf(path, "%s/%s", MAIL_DIR, split[1]);
    if(access(path, F_OK) != 0) mkdir(path, 0700);
    
    char filename[1024];
    //sprintf(filename, "%ld_%s", time(0), split[0]); 
    sprintf(filename, "%s/%s", path, reformat_string(split[2]));
    FILE* file = fopen(filename, "w");
    fputs(content, file);
    fclose(file);

    free_data(&split);
    return make_TW_SERVER_PACKET(SERVER_OK);
}

// https://stackoverflow.com/questions/7666509/hash-function-for-string
// Author: Dan Bernstein
unsigned long hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>


#include "../lib/tw_packet.h"
#include "../lib/tw_utility.h"
#include "../lib/queue.h"

#define MAIL_DIR "./inbox"
// ./inbox/<user>/date_sender

typedef struct session {
    int logged_in;
    char username[9];
} session;

void init();
char** split_data(char* data);
void free_data(char*** split);
TW_PACKET login(char* username, char* password);
TW_PACKET sv_send(session* session, char* content);
TW_PACKET list(session* session);
TW_PACKET sv_read(session* session, int index);
TW_PACKET del(session* session, int index);
int grab_index(TW_PACKET *packet);

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

    session session;
    session.logged_in = 1;
    strcpy(session.username, "wberger");
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

            printf("%d -> %s\n", pktBuf.header, type2str(pktBuf.header));

            TW_PACKET ans;
            switch(pktBuf.header) {
                case SEND: ans = sv_send(&session, pktBuf.data); break;
                case LIST: ans = list(&session); break;
                case DELETE: ans = del(&session, grab_index(&pktBuf)); break;
                case READ: ans = sv_read(&session, grab_index(&pktBuf)); break;
                default: printf("Test123"); break;
            }

            //TW_PACKET ans = make_TW_SERVER_PACKET(SERVER_OK, NULL);
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

int grab_index(TW_PACKET *packet) {
    char** split = split_data(packet->data);
    int ret = atoi(split[2]);

    return ret;
}

// SEND

// 0 SENDER
// 1 RECEIVER
// 2 SUBJ
// 3 MESSAGE
TW_PACKET sv_send(session* session, char* content) {
    char **split = split_data(content);
    char path[1024];

    sprintf(path, "%s/%s", MAIL_DIR, split[1]);
    if(access(path, F_OK) != 0) mkdir(path, 0700);
    
    char filename[2048];
    //sprintf(filename, "%ld_%s", time(0), split[0]); 
    sprintf(filename, "%s/%s", path, reformat_string(split[2]));
    FILE* file = fopen(filename, "w");
    fputs(content, file);
    fclose(file);

    free_data(&split);
    return make_TW_SERVER_PACKET(SERVER_OK, NULL);
}

int cmpfunc(const void *a, const void *b) {
    char* _a = (char*)a;
    char* _b = (char*)b;

    printf("%s vs %s\n", _a, _b);

    printf("%p und %p\n", a, b);

    return tolower(((char*)a)[0]) < tolower(((char*)b)[0]);
}

TW_PACKET list(session* session) {
    DIR *dir;
    struct dirent *dirent;
    queue_t *mail_list = mkqueue(sizeof(char*));

    char path[strlen(MAIL_DIR) + strlen(session->username) + 1];
    sprintf(path, "%s/%s", MAIL_DIR, session->username);

    dir = opendir(path);

    while((dirent = readdir(dir)) != NULL) {
        if(strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0) continue;

        char* mail_name = malloc(strlen(dirent->d_name) + 1);
        strcpy(mail_name, dirent->d_name);
        queue_add(mail_list, mail_name);
    }


    int mail_list_bytes = 0;
    // Sort alphabetically
    int i, j;
    int swapped;
    for (i = 0; i < mail_list->length - 1; i++) {
        mail_list_bytes += strlen((char*)mail_list->queue[i]);
        swapped = 0;
        for (j = 0; j < mail_list->length - i - 1; j++) {
            if (tolower(((char*)mail_list->queue[j])[0]) > tolower(((char*)mail_list->queue[j + 1])[0])) {
                void *temp = mail_list->queue[j];
                mail_list->queue[j] = mail_list->queue[j + 1];
                mail_list->queue[j + 1] = temp;
                swapped = 1;
            }
        }
 
        // If no two elements were swapped by inner loop,
        // then break
        if (!swapped)
            break;
    }

    char mail_list_str[mail_list_bytes + 1];
    memset(mail_list_str, 0, mail_list_bytes + 1);
    for(int i = 0; i < mail_list->length; i++) {
        strcat(mail_list_str, (char*)mail_list->queue[i]);
        strcat(mail_list_str, "\n");
    }

    queue_delete(&mail_list);
    closedir(dir);

    TW_PACKET reply = make_TW_SERVER_PACKET(SERVER_OK, mail_list_str);
    return reply;
}

TW_PACKET del(session* session, int index) {
    DIR *dir;
    struct dirent *dirent;
    queue_t *mail_list = mkqueue(sizeof(char*));

    char path[strlen(MAIL_DIR) + strlen(session->username) + 1];
    sprintf(path, "%s/%s", MAIL_DIR, session->username);

    dir = opendir(path);

    while((dirent = readdir(dir)) != NULL) {
        if(strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0) continue;

        char* mail_name = malloc(strlen(dirent->d_name) + 1);
        strcpy(mail_name, dirent->d_name);
        queue_add(mail_list, mail_name);
    }


    int mail_list_bytes = 0;
    // Sort alphabetically
    int i, j;
    int swapped;
    for (i = 0; i < mail_list->length - 1; i++) {
        mail_list_bytes += strlen((char*)mail_list->queue[i]);
        swapped = 0;
        for (j = 0; j < mail_list->length - i - 1; j++) {
            if (tolower(((char*)mail_list->queue[j])[0]) > tolower(((char*)mail_list->queue[j + 1])[0])) {
                void *temp = mail_list->queue[j];
                mail_list->queue[j] = mail_list->queue[j + 1];
                mail_list->queue[j + 1] = temp;
                swapped = 1;
            }
        }
 
        // If no two elements were swapped by inner loop,
        // then break
        if (!swapped)
            break;
    }

    char mail_list_str[mail_list_bytes + 1];
    memset(mail_list_str, 0, mail_list_bytes + 1);
    for(int i = 0; i < mail_list->length; i++) {
        strcat(mail_list_str, (char*)mail_list->queue[i]);
        strcat(mail_list_str, "\n");
    }

    TW_PACKET reply;
    if(mail_list->length <= index) {
        reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    } else {
        char fullpath[1024];
        sprintf(fullpath, "%s/%s", path, (char*) queue_get(mail_list, index));
        if(remove(fullpath) == -1) reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
        else reply = make_TW_SERVER_PACKET(SERVER_OK, NULL);
    }

    queue_delete(&mail_list);
    closedir(dir);

    return reply;
}

TW_PACKET sv_read(session* session, int index) {
    DIR *dir;
    struct dirent *dirent;
    queue_t *mail_list = mkqueue(sizeof(char*));

    char path[strlen(MAIL_DIR) + strlen(session->username) + 1];
    sprintf(path, "%s/%s", MAIL_DIR, session->username);

    dir = opendir(path);

    while((dirent = readdir(dir)) != NULL) {
        if(strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0) continue;

        char* mail_name = malloc(strlen(dirent->d_name) + 1);
        strcpy(mail_name, dirent->d_name);
        queue_add(mail_list, mail_name);
    }


    int mail_list_bytes = 0;
    // Sort alphabetically
    int i, j;
    int swapped;
    for (i = 0; i < ((int)mail_list->length) - 1; i++) {
        mail_list_bytes += strlen((char*)queue_get(mail_list, i));
        swapped = 0;
        for (j = 0; j < mail_list->length - i - 1; j++) {
            if (tolower(((char*)mail_list->queue[j])[0]) > tolower(((char*)mail_list->queue[j + 1])[0])) {
                void *temp = mail_list->queue[j];
                mail_list->queue[j] = mail_list->queue[j + 1];
                mail_list->queue[j + 1] = temp;
                swapped = 1;
            }
        }
 
        // If no two elements were swapped by inner loop,
        // then break
        if (!swapped)
            break;
    }

    char mail_list_str[mail_list_bytes + 1];
    memset(mail_list_str, 0, mail_list_bytes + 1);
    for(int i = 0; i < mail_list->length; i++) {
        strcat(mail_list_str, (char*)mail_list->queue[i]);
        strcat(mail_list_str, "\n");
    }

    TW_PACKET reply;
    if(mail_list->length <= index) {
        reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    } else {
        char fullpath[1024];
        sprintf(fullpath, "%s/%s/%s", MAIL_DIR, session->username, (char*) queue_get(mail_list, index));
        
        FILE *mail = fopen(fullpath, "r");
        
        printf("Fullpath is %s\n", fullpath);
        if(mail == NULL) reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
        else {
            fseek(mail, 0, SEEK_END);
            long size = ftell(mail);
            fseek(mail, 0, SEEK_SET);

            char mail_content[size + 1];
            memset(mail_content, 0, size + 1);
            fread(mail_content, size, 1, mail);
            reply = make_TW_SERVER_PACKET(SERVER_OK, mail_content);
            fclose(mail);
        }
    }

    queue_delete(&mail_list);
    closedir(dir);

    return reply;
}
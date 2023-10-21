#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../lib/tw_packet.h"
#include "../lib/tw_utility.h"
#include "../lib/queue.h"

#define MAIL_DIR "./inbox"
#define MAX_CONNECTIONS 10
#define PORT 10101

typedef struct connection {
    int socketfd;
    pthread_t thread_id;
} connection;

typedef struct session {
    int logged_in;
    char username[9];
} session;

int base_sockfd = -1;
int abort_requested = 0;
connection connection_pool[MAX_CONNECTIONS];


void init();
void signalHandler(int sig);
void* handle_client(void *conn);
connection* get_connection_slot();

TW_PACKET login(char* username, char* password, session* session);
TW_PACKET sv_send(session* session, char* content);
TW_PACKET list(session* session);
TW_PACKET sv_read(session* session, int index);
TW_PACKET del(session* session, int index);
int grab_index(TW_PACKET *packet);
queue_t* get_mail_list(session* session);

int main(int argc, char *argv[])
{
    init();
    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        perror("signal can not be registered");
        return EXIT_FAILURE;
    }

    int reuseValue = 1;
    socklen_t addrlen;
    struct sockaddr_in address, cliaddress;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((base_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket error");
        return EXIT_FAILURE;
    }

    if (setsockopt(base_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseValue, sizeof(reuseValue)) == -1) {
        perror("set socket options - reuseAddr");
        return EXIT_FAILURE;
    }

    if (setsockopt(base_sockfd, SOL_SOCKET, SO_REUSEPORT, &reuseValue, sizeof(reuseValue)) == -1) {
        perror("set socket options - reusePort");
        return EXIT_FAILURE;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(base_sockfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind error");
        return EXIT_FAILURE;
    }

    if (listen(base_sockfd, 5) == -1) {
        perror("listen error");
        return EXIT_FAILURE;
    }

    while (!abort_requested)
    {
        int new_socket = -1;
        printf("Waiting for connections...\n");
        addrlen = sizeof(struct sockaddr_in);
        if ((new_socket = accept(base_sockfd, (struct sockaddr *)&cliaddress, &addrlen)) == -1)
        {
            if (abort_requested) {
                perror("accept error after aborted");
            } else {
                perror("accept error");
            }
            break;
        }


        connection *new_connection = get_connection_slot();
        if(new_connection == NULL) {
            printf("Client %s:%d tried to connect but server is full...\n", inet_ntoa(cliaddress.sin_addr), ntohs(cliaddress.sin_port));
            continue;
            // Handle client rejection
        }
        printf("Client connected from %s:%d...\n", inet_ntoa(cliaddress.sin_addr), ntohs(cliaddress.sin_port));

        new_connection->socketfd = new_socket;
        pthread_create(&new_connection->thread_id, NULL, handle_client, new_connection);
    }

    for(int i = 0; i < MAX_CONNECTIONS; i++) {
        if(connection_pool[i].thread_id != 0) {
            pthread_join(connection_pool[i].thread_id, NULL);
        }
    }

    return 0;
}

void init() {
    if(access(MAIL_DIR, F_OK) != 0)
        mkdir(MAIL_DIR, 0700);
    
    for(int i = 0; i < MAX_CONNECTIONS; i++) {
        connection_pool[i].socketfd = -1;
        connection_pool[i].thread_id = 0;
    }
}

void signalHandler(int sig)
{
    if (sig == SIGINT)
    {
        printf("Abort Requested...\n");
        abort_requested = 1;

        for(int i = 0; i < MAX_CONNECTIONS; i++) {
            if (connection_pool[i].socketfd != -1)
            {
                if (shutdown(connection_pool[i].socketfd, SHUT_RDWR) == -1)
                {
                    perror("shutdown new_socket");
                }
                if (close(connection_pool[i].socketfd) == -1)
                {
                    perror("close new_socket");
                }
                connection_pool[i].socketfd = -1;
            }
        }

        if (base_sockfd != -1)
        {
            if (shutdown(base_sockfd, SHUT_RDWR) == -1)
            {
                perror("shutdown create_socket");
            }
            if (close(base_sockfd) == -1)
            {
                perror("close create_socket");
            }
            base_sockfd = -1;
        }
    }
    else
    {
        exit(sig);
    }
}

void* handle_client(void *connptr) {
    connection *conn = (connection*) connptr;

    session session;
    session.logged_in = 0;
    TW_PACKET pktBuf;

    char buffer[1024];
    strcpy(buffer, "Welcome to FHTW-Mailer!\r\nPlease enter your commands...\r\n");
    if (send(conn->socketfd, buffer, strlen(buffer), 0) == -1) {
        perror("send failed");
        return NULL;
    }
    
    do {
        pktBuf = receive_TW_PACKET(conn->socketfd);
        printf("Received %ld bytes from %d\n", pktBuf.header.size + sizeof(TW_PACKET_HEADER), conn->socketfd);

        TW_PACKET ans;
        ans.header.type = INVALID;
        switch(pktBuf.header.type) {
            case SEND: ans = sv_send(&session, pktBuf.data); break;
            case LIST: ans = list(&session); break;
            case DELETE: ans = del(&session, grab_index(&pktBuf)); break;
            case READ: ans = sv_read(&session, grab_index(&pktBuf)); break;
            case LOGIN: {
                char** split = split_data(pktBuf.data);
                ans = login(split[1], split[2], &session);
                free_data(&split);
                break;
            }
            default: break;
        }

        if(pktBuf.header.type == QUIT) {
            free_TW_PACKET(&pktBuf);
            break;
        }

        if(ans.header.type == INVALID) ans = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
        
        send_TW_PACKET(conn->socketfd, &ans);
        free_TW_PACKET(&ans);
        free_TW_PACKET(&pktBuf);
    } while(!abort_requested);

    free_TW_PACKET(&pktBuf);
    pthread_exit(NULL);

    conn->socketfd = -1;
    conn->thread_id = 0;

    return NULL;
}

connection* get_connection_slot() {
    connection* conn = NULL;

    for(int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connection_pool[i].socketfd == -1 && connection_pool[i].thread_id == 0) return &connection_pool[i];
    }

    return conn;
}

int grab_index(TW_PACKET *packet) {
    char** split = split_data(packet->data);
    int ret = atoi(split[1]);
    free_data(&split);
    return ret;
}

TW_PACKET login(char* username, char* password, session* session) {
    // Handle ldap login
    strcpy(session->username, username);
    session->logged_in = 1;
    
    // TODO REMOVE
    strcpy(password, "1");

    return make_TW_SERVER_PACKET(SERVER_OK, NULL);
}

// SEND

// 0 RECEIVER
// 1 SUBJ
// 2 MESSAGE
TW_PACKET sv_send(session* session, char* content) {
    if(!session->logged_in) return make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    char **split = split_data(content);
    char path[1024];

    if(!match(split[0], "^([a-z0-9]){1,8}$")) {
        free_data(&split);
        return make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    }

    sprintf(path, "%s/%s", MAIL_DIR, split[0]);
    if(access(path, F_OK) != 0) mkdir(path, 0700);
    
    char filename[2048];
    sprintf(filename, "%s/%s", path, reformat_string(split[1]));
    FILE* file = fopen(filename, "w");
    fputs(content, file);
    fclose(file);

    free_data(&split);
    return make_TW_SERVER_PACKET(SERVER_OK, NULL);
}

queue_t* get_mail_list(session* session) {
    DIR *dir;
    struct dirent *dirent;  
    queue_t *mail_list = mkqueue(sizeof(char*));

    char path[strlen(MAIL_DIR) + strlen(session->username) + 1];
    sprintf(path, "%s/%s", MAIL_DIR, session->username);

    if(access(path, F_OK) != 0) {
        mkdir(path, 0700);
        return mail_list;
    }

    dir = opendir(path);

    while((dirent = readdir(dir)) != NULL) {
        if(strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0) continue;

        char* mail_name = malloc(strlen(dirent->d_name) + 1);
        strcpy(mail_name, dirent->d_name);
        queue_add(mail_list, mail_name);
    }

    if(mail_list->length > 1) {
        // Sort alphabetically
        int i, j;
        int swapped;
        for (i = 0; i < mail_list->length - 1; i++) {
            swapped = 0;
            for (j = 0; j < mail_list->length - i - 1; j++) {
                if(strcasecmp((char*)mail_list->queue[j], (char*)mail_list->queue[j + 1]) > 0) {
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
    }

    closedir(dir);
    return mail_list;
}

TW_PACKET list(session* session) {
    if(!session->logged_in) return make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    queue_t* mail_list = get_mail_list(session);
    if(mail_list->length <= 0) {
        queue_delete(&mail_list);
        return make_TW_SERVER_PACKET(SERVER_OK, NULL);
    }

    int mail_list_bytes = 0;
    for(int i = 0; i < mail_list->length; i++) {
        mail_list_bytes += strlen((char*)queue_get(mail_list, i));
    }

    TW_PACKET reply;

    if(mail_list_bytes == 0) {
        reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    } else {
        char mail_list_str[mail_list_bytes + 1];
        memset(mail_list_str, 0, mail_list_bytes + 1);
        for(int i = 0; i < mail_list->length; i++) {
            strcat(mail_list_str, (char*)mail_list->queue[i]);
            strcat(mail_list_str, "\n");
        }

        reply = make_TW_SERVER_PACKET(SERVER_OK, mail_list_str);
    }

    queue_delete(&mail_list);
    return reply;
}

TW_PACKET del(session* session, int index) {
    if(!session->logged_in) return make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    queue_t* mail_list = get_mail_list(session);
    if(mail_list->length <= 0) {
        queue_delete(&mail_list);
        return make_TW_SERVER_PACKET(SERVER_OK, NULL);
    }

    TW_PACKET reply;
    if(mail_list->length <= index) {
        reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    } else {
        char fullpath[1024];
        sprintf(fullpath, "%s/%s/%s", MAIL_DIR, session->username, (char*) queue_get(mail_list, index));

        if(remove(fullpath) == -1) reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
        else reply = make_TW_SERVER_PACKET(SERVER_OK, NULL);
    }

    queue_delete(&mail_list);
    return reply;
}

TW_PACKET sv_read(session* session, int index) {
    if(!session->logged_in) return make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    queue_t* mail_list = get_mail_list(session);

    TW_PACKET reply;
    if(mail_list->length <= index) {
        reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    } else {
        char fullpath[1024];
        sprintf(fullpath, "%s/%s/%s", MAIL_DIR, session->username, (char*) queue_get(mail_list, index));
        
        FILE *mail = fopen(fullpath, "r");
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
    return reply;
}
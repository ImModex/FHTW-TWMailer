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
#include <termios.h>
#include <time.h>

#include "../lib/tw_packet.h"
#include "../lib/tw_utility.h"
#include "../lib/queue.h"
#include "../lib/myldap.h"

char* MAIL_DIR = "./inbox";

#define LOGIN_ATTEMPT_FILE "logins.csv"
#define MAX_CONNECTIONS 10

// Login attempt with timestamp, how many previous attempts and IP address
typedef struct login_attempt {
    time_t timestamp;
    unsigned int attempts;
    char address[16];
} login_attempt;

// Client connection with socketID and threadID
typedef struct connection {
    int socketfd;
    char address[16];
    pthread_t thread_id;
    queue_t* login_attempt_cache;
    pthread_mutex_t *lock;
} connection;

// I don't think this is needed but holds login status and username
typedef struct session {
    int logged_in;
    char username[9];
} session;

int base_sockfd = -1;                           // Base server socket
int abort_requested = 0;                        // Abort flag
connection connection_pool[MAX_CONNECTIONS];    // Connected clients

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

login_attempt* get_login_attempt(queue_t* login_attempt_cache, char* address);
login_attempt* create_login_attempt(time_t timestamp, int attempts, char* address);
void save_login_attempts(const char* filename, queue_t* login_attempt_cache);
queue_t* load_login_attempts(const char* filename);

int main(int argc, char *argv[])
{
    // Check input
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <spool-dir>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pthread_mutex_t lock;
    MAIL_DIR = argv[2];

    // Run initial tasks
    init();

    // Register signal handler
    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        perror("signal can not be registered");
        return EXIT_FAILURE;
    }

    // Initialize mutex for thread synchronization
    if (pthread_mutex_init(&lock, NULL) != 0) { 
        perror("mutex init has failed\n"); 
        return EXIT_FAILURE; 
    }

    // Open socket, bind it to a port/ip and start listening
    int reuseValue = 1;
    socklen_t addrlen;
    struct sockaddr_in address, cliaddress;

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
    address.sin_port = htons(atoi(argv[1]));

    if (bind(base_sockfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind error");
        return EXIT_FAILURE;
    }

    if (listen(base_sockfd, 5) == -1) {
        perror("listen error");
        return EXIT_FAILURE;
    }

    printf("Waiting for connections...\n");

    // Load login attempts from file and populate cache
    queue_t* login_attempt_cache = load_login_attempts(LOGIN_ATTEMPT_FILE);
    while (!abort_requested)
    {
        int new_socket = -1;        
        addrlen = sizeof(struct sockaddr_in);
        
        // Accept clients trying to connect
        if ((new_socket = accept(base_sockfd, (struct sockaddr *)&cliaddress, &addrlen)) == -1)
        {
            if (!abort_requested) perror("accept error");
            break;
        }

        // Handle blacklisted IP
        char* address = inet_ntoa(cliaddress.sin_addr);
        login_attempt *attempt = get_login_attempt(login_attempt_cache, address);
        if(attempt && attempt->attempts >= 3) {
            printf("Blacklisted client %s tried to connect and was rejected.\n", address);
            shutdown(new_socket, SHUT_RDWR);
            close(new_socket);
            continue;
        }

        // Fetch a connection slot
        connection *new_connection = get_connection_slot();

        // If no slot is available, reject connection attempt
        if(new_connection == NULL) {
            printf("Client %s:%d tried to connect but server is full...\n", address, ntohs(cliaddress.sin_port));
            continue;
            // Handle client rejection
        }
        printf("Client connected from %s:%d...\n", address, ntohs(cliaddress.sin_port));

        // Start a new thread for every connected client and handle requests
        new_connection->socketfd = new_socket;
        strcpy(new_connection->address, address);
        new_connection->lock = &lock;
        new_connection->login_attempt_cache = login_attempt_cache;
        pthread_create(&new_connection->thread_id, NULL, handle_client, new_connection);
    }

    // Wait for all client threads to be done before exiting
    for(int i = 0; i < MAX_CONNECTIONS; i++) {
        if(connection_pool[i].thread_id != 0) {
            pthread_join(connection_pool[i].thread_id, NULL);
        }
    }

    queue_delete(&login_attempt_cache);
    pthread_mutex_destroy(&lock);
    return 0;
}

// Create mail spool directory and initialize connection pool
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
    if (sig == SIGINT) {
        printf("Abort Requested...\n");
        abort_requested = 1;

        for(int i = 0; i < MAX_CONNECTIONS; i++) {
            if (connection_pool[i].socketfd != -1) {
                if (shutdown(connection_pool[i].socketfd, SHUT_RDWR) == -1) {
                    perror("shutdown new_socket");
                }
                if (close(connection_pool[i].socketfd) == -1) {
                    perror("close new_socket");
                }
                connection_pool[i].socketfd = -1;
            }
        }

        if (base_sockfd != -1)
        {
            // I think this is not needed since the base socket only accepts connections and
            // assigns them a new sockfd, therefore this is a UDP protocol and does not
            // need to be shut down?
            /*
            if (shutdown(base_sockfd, SHUT_RDWR) == -1) {
                perror("shutdown create_socket");
            }
            */
            if (close(base_sockfd) == -1) {
                perror("close create_socket");
            }
            base_sockfd = -1;
        }
    } else {
        exit(sig);
    }
}

// Handle a client connection
void* handle_client(void *connptr) {
    // Client-owned connection object
    connection *conn = (connection*) connptr;

    int thread_abort_requested = 0;
    
    session session = {
        .username = {0},
        .logged_in = 0
    };

    TW_PACKET pktBuf;

    // Send server-hello
    char buffer[1024];
    memset(buffer, 0, 1024);
    strcpy(buffer, "Welcome to FHTW-Mailer!\r\nPlease enter your commands...\r\n");
    if (send(conn->socketfd, buffer, strlen(buffer), 0) == -1) {
        perror("send failed");
        return NULL;
    }
    
    // Command handling loop
    do {
        // Receive data
        pktBuf = receive_TW_PACKET(conn->socketfd);
        
        // Only allow one write operation to console at once
        pthread_mutex_lock(conn->lock);
        printf("Received %ld bytes from %d\n", pktBuf.header.size + sizeof(TW_PACKET_HEADER), conn->socketfd);
        pthread_mutex_unlock(conn->lock);

        // Read packet header and call corresponding function to generate an answer
        TW_PACKET ans;
        ans.header.type = INVALID;

        // All command operations depend on files / shared data so it is synchronized by locking the mutex
        pthread_mutex_lock(conn->lock);
        switch(pktBuf.header.type) {
            case SEND: ans = sv_send(&session, pktBuf.data); break;
            case LIST: ans = list(&session); break;
            case DELETE: ans = del(&session, grab_index(&pktBuf)); break;
            case READ: ans = sv_read(&session, grab_index(&pktBuf)); break;
            case LOGIN: {
                char** split = split_data(pktBuf.data);
                ans = login(split[1], split[2], &session);
                
                // If login fails, get login attempt and add one. If it is the first attempt, create one.
                // When the maximum retry amount is exceeded (3), stop the thread and close the connection
                if(ans.header.type == SERVER_ERR) {

                    login_attempt* attempt = get_login_attempt(conn->login_attempt_cache, conn->address);
                    if(attempt == NULL) {
                        attempt = create_login_attempt(time(0), 1, conn->address);
                        queue_add(conn->login_attempt_cache, attempt);
                    } else {
                        attempt->attempts++;
                        attempt->timestamp = time(0);
                        
                        if(attempt->attempts >= 3) {
                            thread_abort_requested = 1;
                        }
                    }

                    save_login_attempts(LOGIN_ATTEMPT_FILE, conn->login_attempt_cache);
                }

                free_data(&split);
                break;
            }
            default: break;
        }
        pthread_mutex_unlock(conn->lock);

        if(pktBuf.header.type == QUIT) {
            free_TW_PACKET(&pktBuf);
            break;
        }

        // Return ERR on invalid packet header
        if(ans.header.type == INVALID) ans = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
        
        if(thread_abort_requested) {
            ans = make_TW_SERVER_PACKET(QUIT, "Maximum login attempts reached...");
        }

        // Send response and free memory
        send_TW_PACKET(conn->socketfd, &ans);
        free_TW_PACKET(&ans);
        free_TW_PACKET(&pktBuf);
    } while(!abort_requested && !thread_abort_requested);


    // After client closed connection, free memory, do thread cleanup and reset connection slot

    free_TW_PACKET(&pktBuf);
    pthread_exit(NULL);

    shutdown(conn->socketfd, SHUT_RDWR);
    close(conn->socketfd);

    conn->socketfd = -1;
    conn->thread_id = 0;

    return NULL;
}

// Gets a login attempt for a specified address from the cache, returns NULL if there is none
login_attempt* get_login_attempt(queue_t* login_attempt_cache, char* address) {
    for(int i = 0; i < login_attempt_cache->length; i++) {
        login_attempt *elem = (login_attempt*) queue_get(login_attempt_cache, i);
        if(strcmp(elem->address, address) == 0) {
            time_t now = time(0);

            if(elem->timestamp + 60 <= now) {
                elem->attempts = 0;
                save_login_attempts(LOGIN_ATTEMPT_FILE, login_attempt_cache);
            }

            return elem;
        }
    }

    return NULL;
}

// Create new login attempt
login_attempt* create_login_attempt(time_t timestamp, int attempts, char* address) {
    login_attempt *attempt = malloc(sizeof(login_attempt));

    attempt->timestamp = timestamp;
    attempt->attempts = attempts;
    strcpy(attempt->address, address);

    return attempt;
}


// CSV format -> timestamp,attempts,ip
// Save login attempt cache to file
void save_login_attempts(const char* filename, queue_t* login_attempt_cache) {
    FILE *file = fopen(filename, "w+");
    if(file == NULL) return;

    for(int i = 0; i < login_attempt_cache->length; i++) {
        login_attempt* attempt = queue_get(login_attempt_cache, i);
        fprintf(file, "%ld,%d,%s\n", attempt->timestamp, attempt->attempts, attempt->address);
    }

    fclose(file);
    return;
}

// Load login attempt cache from file
queue_t* load_login_attempts(const char* filename) {
    FILE *file = fopen(filename, "r+");
    if(file == NULL) {
        file = fopen(filename, "w+");
        fclose(file);
        return mkqueue(sizeof(login_attempt));
    }

    queue_t* login_attempt_cache = mkqueue(sizeof(login_attempt));
    
    enum ParseState {
        TIMESTAMP,
        ATTEMPTS,
        ADDRESS
    };

    enum ParseState state;
    char* token;
    char buf[100];

    // Read file line-by-line
    while (fgets(buf, 100, file)) {
        login_attempt tmp = {
            .address = {0}
        };

        state = TIMESTAMP;
        token = strtok(buf, ","); // Split data on ','
        while(token) {
            switch(state) {
                case TIMESTAMP: tmp.timestamp = atol(token); break;
                case ATTEMPTS: tmp.attempts = atoi(token); break;
                case ADDRESS: strncpy(tmp.address, token, strlen(token)-1); break;
                default: break; 
            }

            state++;
            token = strtok(NULL, ",");
        }

        queue_add(login_attempt_cache, create_login_attempt(tmp.timestamp, tmp.attempts, tmp.address));
    }
    
    fclose(file);
    return login_attempt_cache;
}

// Returns the address of a free connection slot or NULL if there is none
connection* get_connection_slot() {
    connection* conn = NULL;

    for(int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connection_pool[i].socketfd == -1 && connection_pool[i].thread_id == 0) return &connection_pool[i];
    }

    return conn;
}

// Get the index from a packet (e.g. READ)
int grab_index(TW_PACKET *packet) {
    char** split = split_data(packet->data);
    int ret = atoi(split[1]);
    free_data(&split);
    return ret;
}

// Handles ldap login
TW_PACKET login(char* username, char* password, session* session) {
    if(ldapConnection(username, password) != 0){
        return make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    }

    strcpy(session->username, username);
    session->logged_in = 1;

    return make_TW_SERVER_PACKET(SERVER_OK, NULL);
}

// SEND
// 0 RECEIVER
// 1 SUBJECT
// 2 MESSAGE
TW_PACKET sv_send(session* session, char* content) {
    // If not logged in, return ERR
    if(!session->logged_in) return make_TW_SERVER_PACKET(SERVER_ERR, NULL);

    char **split = split_data(content);

    // If receiver username is invalid, return ERR
    if(!match(split[0], "^([a-z0-9]){1,8}$")) {
        free_data(&split);
        return make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    }

    // Build path of receiver's mail directory
    char path[1024];
    sprintf(path, "%s/%s", MAIL_DIR, split[0]);
    if(access(path, F_OK) != 0) mkdir(path, 0700); // Create it if it doesn't exist
    
    // Create E-Mail file and write it's content
    char filename[2048];
    sprintf(filename, "%s/%s", path, reformat_string(split[1]));
    FILE* file = fopen(filename, "w");
    fprintf(file, "%s\n%s", session->username, (content + strlen(split[0]) * sizeof(char) + 1));
    fclose(file);

    free_data(&split);
    return make_TW_SERVER_PACKET(SERVER_OK, NULL);
}

// Returns an alphabetically sorted list of E-Mails for a given user
queue_t* get_mail_list(session* session) {
    DIR *dir;
    struct dirent *dirent;
    queue_t *mail_list = mkqueue(sizeof(char*));

    char path[strlen(MAIL_DIR) + strlen(session->username) + 1];
    sprintf(path, "%s/%s", MAIL_DIR, session->username);

    // Create user's mail directory if it doesn't exist
    if(access(path, F_OK) != 0) {
        mkdir(path, 0700);
        return mail_list;
    }

    dir = opendir(path);

    // Read all file contents and add it to the queue
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
    
            if (!swapped) break;
        }
    }

    closedir(dir);
    return mail_list;
}

// List all emails of given user
TW_PACKET list(session* session) {
    // If not logged in, return ERR
    if(!session->logged_in) return make_TW_SERVER_PACKET(SERVER_ERR, NULL);

    // Get list of emails
    queue_t* mail_list = get_mail_list(session);

    // Return OK and empty list if there is no E-Mails
    if(mail_list->length <= 0) {
        queue_delete(&mail_list);
        return make_TW_SERVER_PACKET(SERVER_OK, NULL);
    }

    // Count needed bytes for all E-Mails
    int mail_list_bytes = 0;
    for(int i = 0; i < mail_list->length; i++) {
        mail_list_bytes += strlen((char*)queue_get(mail_list, i));
    }

    TW_PACKET reply;

    if(mail_list_bytes == 0) {
        reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    } else {
        // Build response containing all mail subjects seperated by \n
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

// Delete E-Mail of given index for a given user
TW_PACKET del(session* session, int index) {
    // If not logged in or index is negative, return ERR
    if(!session->logged_in || index < 0) return make_TW_SERVER_PACKET(SERVER_ERR, NULL);

    // Get all E-Mails, return ERR if there is none
    queue_t* mail_list = get_mail_list(session);
    if(mail_list->length <= 0) {
        queue_delete(&mail_list);
        return make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    }

    // If there is no E-Mail for given index, return ERR
    TW_PACKET reply;
    if(mail_list->length <= index) {
        reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    } else {
        // Build path of E-Mail file and delete it
        char fullpath[1024];
        sprintf(fullpath, "%s/%s/%s", MAIL_DIR, session->username, (char*) queue_get(mail_list, index));

        if(remove(fullpath) == -1) reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
        else reply = make_TW_SERVER_PACKET(SERVER_OK, NULL);
    }

    queue_delete(&mail_list);
    return reply;
}

// Read E-Mail of given index for a given user
TW_PACKET sv_read(session* session, int index) {
    // If not logged in or index is negative, return ERR
    if(!session->logged_in || index < 0) return make_TW_SERVER_PACKET(SERVER_ERR, NULL);

    // Get all E-Mails
    queue_t* mail_list = get_mail_list(session);

    // If there is no E-Mail for given index, return ERR
    TW_PACKET reply;
    if(mail_list->length <= index) {
        reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
    } else {
        // Build path of E-Mail file
        char fullpath[1024];
        sprintf(fullpath, "%s/%s/%s", MAIL_DIR, session->username, (char*) queue_get(mail_list, index));
        
        FILE *mail = fopen(fullpath, "r");
        if(mail == NULL) reply = make_TW_SERVER_PACKET(SERVER_ERR, NULL);
        else {
            // Get length of E-Mail content
            fseek(mail, 0, SEEK_END);
            long size = ftell(mail);
            fseek(mail, 0, SEEK_SET);

            // Write content to a buffer and return it in a packet
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
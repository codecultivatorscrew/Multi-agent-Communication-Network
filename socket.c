#include "socket.h"

#define PORT 6001
#define SERVER_PORT 5001
#define SERVER_IP "localhost"
#define BACKLOG 100
#define BUFFER_SIZE 1024

struct listen_thread_arg{
    Callback cb;
    int socket_fd;
};

void *listen_thread_func (void* in_args){
    //unpack thread argument
    struct listen_thread_arg *args= (struct listen_thread_arg*) in_args;
    int socket_fd = args->socket_fd;
    Callback callback_func = args->cb;

    printf("Listening on port %d...\n", PORT);
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    
    while(1){        
        char buffer[BUFFER_SIZE] = {0};
        // Receive data from client
        if (read(socket_fd, buffer, BUFFER_SIZE) == -1){
            printf("failed\n");
        }

        printf("Recive socket message: %s\n", buffer);
        callback_func(buffer);
    }

    // // Close sockets
    // close(socket_fd);
}

pthread_t init_socket( Callback cb){ //return socket_fd so it can be closed later
    int socket_fd;
    connect_socket(&socket_fd);

    // spawn a thread
    pthread_t tid;
    struct listen_thread_arg* args = (struct listen_thread_arg*)malloc(sizeof(struct listen_thread_arg));
    args->cb = cb;
    args->socket_fd = socket_fd;
    pthread_create(&tid, NULL, listen_thread_func, (void*)args);
    
    // need to close socket_fd outside
    // struct  init_socket_return_type ret;
    // ret.socket_fd =socket_fd;
    // ret.tid = tid;
    return tid;
}

int socket_sent(char* message, size_t length) {
    static int send_socket_fd = -1;
    static struct sockaddr_in server_addr;

    if (send_socket_fd == -1) {
        printf("Send socket connected to server\n");
        connect_socket(&send_socket_fd);
    }
    
    
    // Send data to server
    // send(send_socket_fd, message, length, MSG_DONTWAIT);
    int byte_sent = send(send_socket_fd, message, length, 0);
    printf("Message sent to server.\n");
    
    // close(send_socket_fd);
    return byte_sent;
}

void connect_socket(int *socket_fd) {
    int new_socket_fd;
    struct sockaddr_in server_addr;
    if ((new_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Prepare server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    *socket_fd = -1;
    // connect network server
    fprintf(stderr, "try to conenct\n");
    if (connect(new_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }
    
    *socket_fd = new_socket_fd;
}
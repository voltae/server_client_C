/**
 * @author Valentin Platzgummer - ic17b096
 * @date 18.11.18
 *
 * @brief Implementation of a simple server Application to work with a client
 * TCP/IP Lecture Distributed Systems
 */
#include <sys/types.h>
#include <sys/socket.h>     // provides socket
#include <errno.h>          // provides errno
#include <stdio.h>          // provides the printf()
#include <stdlib.h>         // provides exit(), EXIT_FAILURE
#include <string.h>         // provide strerror(), strlen()
#include <arpa/inet.h>      // provides inet_address
#include <unistd.h>         // provides read(), write(), close()

/*@TODO Replace testing addresses with user defined from args */
#define ADDRESS "127.0.0.1"
#define PORT 7329
#define RECEIVERBUFFER 100
/* Server needs following syscalls
 * socket() x
 * bind()   x
 * listen() x
 * accept()
 * read()
 * write()
 * close()
 */
/*@TODO Replace testing addresses with user defined from args */
#define ADDRESS "127.0.0.1"
#define PORT 7329
#define RECEIVERBUFFER 100
#define BACKLOG 5

static void errorMessage(char* userMessage, char* errorMessage, const char* progname);

int main(int argc, const char** argv) {
    char buffer[RECEIVERBUFFER] = {"\0"};
    char messageServer[] = "The Server is saying annoying things.";
    // file descriptor socket server
    int fd_socket_server, fd_socket_client;
    struct sockaddr_in server_add, client_add;  // Server Socket, Client Socket
    const char* prog = argv[0];
    // SOCKET()
    fd_socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket_server < 0) {
        errorMessage("Could not create a socket: ", strerror(errno), prog);
    }

    // initialize struct with 0
    memset(&server_add, 0, sizeof(server_add));
    server_add.sin_family = AF_INET;
    server_add.sin_port = htons(PORT);
    /* Convert Internet host address from numbers-and-dots notation in CP
   into binary data in network byte order.  */
    unsigned long address;
    address = inet_addr(ADDRESS);
    // Memcopy the casted address
    memcpy((char*) &server_add.sin_addr, &address, sizeof(address));
    // BIND()
    if (bind(fd_socket_server, (struct sockaddr*) &server_add, sizeof(server_add)) < 0) {
        errorMessage("Could not bind to socket: ", strerror(errno), prog);
    }
    //fprintf(stdout, "Server Socket:%s:%d created.\n", server_add.sin_addr, ntohs(server_add.sin_port));
    // LISTEN()
    if (listen(fd_socket_server, BACKLOG) < 0) {
        errorMessage("Could not listen to socket: ", strerror(errno), prog);
    }
    fprintf(stdout, "Server Socket created. Waiting ...\n");

    // Endless loop, Server must be killed manually
    socklen_t len_client = sizeof(client_add);
    int rec_size;   // bytes received from client
    while (1) {
        fd_socket_client = accept(fd_socket_server, (struct sockaddr*) &client_add, &len_client);
        if (fd_socket_client < 0) {
            errorMessage("Could not accept socket: ", strerror(errno), prog);
        }

        rec_size = read(fd_socket_client, buffer, RECEIVERBUFFER);
        if (rec_size < 0) {
            errorMessage("Could not read from Client: ", strerror(errno), prog);
        }
        fprintf(stdout, "Client says: %s\n", buffer);

        write(fd_socket_client, messageServer, strlen(messageServer));
    }
}

/**
 * Print out the error message for th user
 * @param userMessage message appropriate for the user
 * @param errorMessage message from the system
 * @param progname provided from the args
 */
static void errorMessage(char* userMessage, char* errorMessage, const char* progname) {
    fprintf(stderr, "%s: %s %s", progname, userMessage, errorMessage);
    exit(EXIT_FAILURE);
}
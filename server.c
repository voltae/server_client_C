/**
 * @author Valentin Platzgummer - ic17b096
 * @date 18.11.18
 *
 * @brief Implementation of a simple server Application to work with a client
 * TCP/IP Lecture Distributed Systems
 */
#include <stdlib.h>         // provides exit(), EXIT_FAILURE
#include <stdio.h>          // provides the printf()
#include <string.h>         // provide strerror(), strlen()
#include <sys/types.h>
#include <sys/socket.h>     // provides socket
#include <errno.h>          // provides errno
#include <arpa/inet.h>      // provides inet_address
#include <unistd.h>         // provides read(), write(), close()
#include <wait.h>           // provides waitpid()

/*@TODO Replace testing addresses with user defined from args
 * Ihr Client soll genauso wie die Musterimplementierung (simple_message_client(1))
 * sowohl mit IPV4 als auch mit IPV6 funktionieren. */
#define RECEIVERBUFFER 100

/*@TODO Replace testing addresses with user defined from args */

#define RECEIVERBUFFER 100
#define BACKLOG 5

static void errorMessage(char* userMessage, char* errorMessage, const char* progname);

static void usage(FILE* stream, const char* cmnd, int exitcode);

static void evaluateParameters(int argc, char* const* argv, u_int16_t* port);

//static void sigchild_handler(int s);

int main(int argc, char* const* argv) {
    char buffer[RECEIVERBUFFER] = {"\0"};
    char messageServer[] = "The Server is saying annoying things.";

    // file descriptor socket server

    int fd_socket_server, fd_socket_client;
    struct sockaddr_in server_add, client_add;  // Server Socket, Client Socket
    const char* progname = argv[0];
    struct sigaction signalact;
    uint16_t port = 0;                          // Initialize Port Variable for Parameter check
    // char clientAdress[INET_ADDRSTRLEN];         // Server address for logging

    evaluateParameters(argc, argv, &port);
    // SOCKET()
    fd_socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket_server < 0) {
        errorMessage("Could not create a socket: ", strerror(errno), progname);
    }

    // initialize struct with 0
    memset(&server_add, 0, sizeof(server_add));
    server_add.sin_family = AF_INET;
    server_add.sin_port = htons(port);
    /* Convert Internet host address from numbers-and-dots notation in CP
   into binary data in network byte order.  */
    server_add.sin_addr.s_addr = htonl(INADDR_ANY);

    // BIND()
    if (bind(fd_socket_server, (struct sockaddr*) &server_add, sizeof(server_add)) < 0) {
        errorMessage("Could not bind to socket: ", strerror(errno), progname);
    }
    fprintf(stdout, "Server Socket:%d created.\n",
            ntohs(server_add.sin_port));
    // LISTEN()
    if (listen(fd_socket_server, BACKLOG) < 0) {
        errorMessage("Could not listen to socket: ", strerror(errno), progname);
    }

    fprintf(stdout, "Server listen on Port :%d.\n",
            ntohs(server_add.sin_port));
    fprintf(stdout, "Server listening. Waiting ...\n");

    // add the child handler to the address struct
    signalact.sa_handler = sigchild_handler;        // let the child action be handled by function
    sigemptyset(&signalact.sa_mask);
    signalact.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &signalact, NULL) == -1) {
        close(fd_socket_server);
        errorMessage("Error in Child process", strerror(errno), progname);
    }
    // Endless loop, Server must be killed manually
    socklen_t len_client = sizeof(client_add);
    int rec_size;   // bytes received from client

    //@TODO Server should act as spawning Server
    // Die Businesslogic des Servers brauchen Sie nicht zu implementieren.
    // - Verwenden Sie das fertige Executable simple_message_server_logic(1) im Sinne eines spawning servers.
    while (1) {
        fd_socket_client = accept(fd_socket_server, (struct sockaddr*) &client_add, &len_client);
        if (fd_socket_client < 0) {
            errorMessage("Could not accept socket: ", strerror(errno), progname);
        }
        /*
        fprintf(stdout, "Got connection from:%d\n",
                ntohs(client_add.sin_addr.s_addr));
        if (!fork()) {
            // child process routine
            // Close the listening socket
            (void) close(fd_socket_server);
            (void) close(fd_socket_client);

            // Redirect the STDIN to the socket
            if (fd_socket_client != STDIN_FILENO) {
                int statusDupRead = dup2(fd_socket_client, STDIN_FILENO);
                if (statusDupRead == -1) {
                    (void) close(fd_socket_client);
                    (void) close(fd_socket_server);
                    errorMessage("Could not redirect the socket", strerror(errno), progname);
                }
            }
            // REdirect the STDOUT to the socket
            if (fd_socket_client != STDOUT_FILENO) {
                int statusDupWrite = dup2(fd_socket_client, STDOUT_FILENO);
                if (statusDupWrite == -1) {
                    (void) close(fd_socket_client);
                    (void) close(fd_socket_server);
                }
            }

            // *** Do the exec here ***
        }
        */
        rec_size = read(fd_socket_client, buffer, RECEIVERBUFFER);
        if (rec_size < 0) {
            errorMessage("Could not read from Client: ", strerror(errno), progname);
        }
        fprintf(stdout, "Client says: %s\n", buffer);

        int success = write(fd_socket_client, messageServer, strlen(messageServer));
        if (success == -1) {
            errorMessage("Could not write to Client.", strerror(errno), progname);
        }
    }
}

static void sigchild_handler(int s) {
    int save_errno = errno;
    // wait for terminating properly the child process
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = save_errno;

}

static void evaluateParameters(int argc, char* const* argv, u_int16_t* port) {
    int opt;
    char* endpointer = NULL;
    int tempPort = 0;
    /* check the parameters from command line */
    // check if any parametersa given
    if (argc < 2) {
        usage(stderr, argv[0], 1);
    }
    while ((opt = getopt(argc, argv, "hp:")) != -1) {
        switch (opt) {
            case 'p':
                tempPort = (int) strtol(optarg, &endpointer, 10);
                if ((*endpointer != 0) || (tempPort < 0 || tempPort > 65535)) {
                    usage(stderr, "wrong port range", 1);
                }
                *port = (uint16_t) tempPort;       // check passed
                break;
            case 'h':
                usage(stdout, argv[0], 0);
                break;
            default:
                usage(stderr, argv[0], 1);
                break;
        }
    }

}

static void errorMessage(char* userMessage, char* errorMessage, const char* progname) {
    fprintf(stderr, "%s: %s %s\n", progname, userMessage, errorMessage);
    exit(EXIT_FAILURE);
}

/**
 * @brief is called upon failure and displays a userinform to stderr and terminates afterwards.
 * @param stream the  stream to write the usage information to (e.g., stdout
                   or stderr).
 * @param cmnd a string containing the name of the executable  (i.e.,  the
                   contents of argv[0]).
 * @param exitcode the  exit code to be used in the call to exit(3) for termi-
                   nating the program.
 */
static void usage(FILE* stream, const char* cmnd, int exitcode) {
    fprintf(stream, "usage: %s option\n", cmnd);
    fprintf(stream, "\t-p, --port <port>\n");
    fprintf(stream, "\t-h, --help\n");
    exit(exitcode);
}
/* usage: simple_message_server option
options:
	-p, --port <port>
	-h, --help
*/
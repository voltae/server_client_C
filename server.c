/**
 * @author Valentin Platzgummer - ic17b096
 * @date 18.11.18
 *
 * @brief Implementation of a simple server Application to work with a client
 * TCP/IP Lecture Distributed Systems
 */

// -------------------------------------------------------------- includes --
#include <stdlib.h>         // provides exit(), EXIT_FAILURE
#include <stdio.h>          // provides the printf()
#include <string.h>         // provide strerror(), strlen()
#include <sys/types.h>
#include <sys/socket.h>     // provides socket
#include <errno.h>          // provides errno
#include <arpa/inet.h>      // provides inet_address
#include <unistd.h>         // provides read(), write(), close()
#include <wait.h>           // provides waitpid()
#include <netdb.h>

// --------------------------------------------------------------- defines --
/**
 * @def Line output. note all log notes must be on stderr, because stdout
 * redirected to the socket
 */
#define LINEOUTPUT fprintf(stdout, "[%s, %s, %d]: ",  __FILE__, __func__, __LINE__)

/**
 * @def maximal amount of requests on listening socket
 */
#define BACKLOG 5
/**
 * @def absolute path to the business logic
 */
#define LOGICS_PATH "/usr/local/bin/simple_message_server_logic"
/**
 * @def name of the business logic application called in this function
 */
#define LOGICS_NAME "simple_message_server_logic"


// -------------------------------------------------------------- typedefs --
/**
 * @brief Struct holds all needed ressources, both file descriptors
 */
typedef struct ressourcesContainer {
    int fd_socket_listen;        /**< File descriptor for the listening socket */
    int fd_socket_connected;     /**< File descriptor for the connected socket */
    const char* progname;        /**< Progamm name argv[0] */
} ressources;


// ------------------------------------------------------------- functions --
static void errorMessage(char* userMessage, char* errorMessage, ressources serverRessources);

static void usage(FILE* stream, const char* cmnd, int exitcode);

static void closeRessources(ressources res);

static void printAddress(struct sockaddr_in sockaddr);

static void evaluateParameters(int argc, char* const* argv, u_int16_t* port, int* verbose);

static void sigchild_handler(int s);


/**
 * @brief main function of the server implementation. Server works as a spawning server. Every connenction is handled
 * by a child process
 * @param argc int: number of incoming paramters
 * @param argv char**: pointerarray with all incoming paramters
 * @return nothing, does not stop
 */
int main(int argc, char* const* argv) {

    // file descriptor socket server
    ressources serverRessources;
    serverRessources.fd_socket_listen = -1; // initialize the file descriptors
    serverRessources.fd_socket_connected = -1;
    serverRessources.progname = argv[0];

    struct sockaddr_in server_add, client_add;  // Server Socket, Client Socket
    struct sigaction signalact;
    uint16_t port = 0;                          // Initialize Port Variable for Parameter check
    int verbose = 0;

    evaluateParameters(argc, argv, &port, &verbose);
    // SOCKET()
    serverRessources.fd_socket_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (serverRessources.fd_socket_listen < 0) {
        errorMessage("Could not create a socket: ", strerror(errno), serverRessources);
    }

    // initialize struct with 0
    memset(&server_add, 0, sizeof(server_add));
    server_add.sin_family = AF_INET;
    server_add.sin_port = htons(port);
    /* Convert Internet host address from numbers-and-dots notation in CP
   into binary data in network byte order.  */
    server_add.sin_addr.s_addr = htonl(INADDR_ANY);

    // Set socket options to reuse address
    int retval;
    int optval = 1;     // set reuse adress to 1;
    retval = setsockopt(serverRessources.fd_socket_listen, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (retval == -1) {
        errorMessage("Reuse address failed", strerror(errno), serverRessources);
    }

    // BIND()
    if (bind(serverRessources.fd_socket_listen, (struct sockaddr*) &server_add, sizeof(server_add)) < 0) {
        errorMessage("Could not bind to socket: ", strerror(errno), serverRessources);
    }
    fprintf(stdout, "Server Socket:%d created.\n",
            ntohs(server_add.sin_port));
    // LISTEN()
    if (listen(serverRessources.fd_socket_listen, BACKLOG) < 0) {
        errorMessage("Could not listen to socket: ", strerror(errno), serverRessources);
    }
    if (verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Server listen on: ");
        printAddress(server_add);
        fprintf(stdout, "Server listening. Waiting ...\n");
    }
    // add the child handler to the address struct
    signalact.sa_handler = sigchild_handler;        // let the child action be handled by function
    sigemptyset(&signalact.sa_mask);
    signalact.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &signalact, NULL) == -1) {
        errorMessage("Error in Child signal process", strerror(errno), serverRessources);
    }
    // Endless loop, Server must be killed manually
    socklen_t len_client = sizeof(client_add);

    // start the spawning server routine, main loop
    while (1) {
        serverRessources.fd_socket_connected = accept(serverRessources.fd_socket_listen, (struct sockaddr*) &client_add,
                                                      &len_client);
        if (serverRessources.fd_socket_connected < 0) {
            errorMessage("Could not accept socket: ", strerror(errno), serverRessources);
        }
        if (verbose == 1) {
            LINEOUTPUT;
            fprintf(stdout, "Got connection from: ");
            printAddress(client_add);
        }
        // Forking a new process
        int fork_return = fork();
        // ------ ERROR CASE --------
        if (fork_return == -1) {
            // child process routine
            // Close the listening socket
            errorMessage("fork failed!", strerror(errno), serverRessources);
        }
            // ------- CHILD PART --------
        else if (fork_return == 0) {
            // Child process
            // Redirect the STDIN to the socket
            if (verbose == 1) {
                LINEOUTPUT;
                fprintf(stdout, "Child process, forking done\n");   // prints must be done to stderr,
            }
            if (serverRessources.fd_socket_connected != STDIN_FILENO) {
                int statusDupRead = dup2(serverRessources.fd_socket_connected, STDIN_FILENO);
                if (statusDupRead == -1) {
                    errorMessage("Could not redirect the read socket", strerror(errno), serverRessources);
                }
            }

            // Redirect the STDOUT to the socket
            if (serverRessources.fd_socket_connected != STDOUT_FILENO) {
                int statusDupWrite = dup2(serverRessources.fd_socket_connected, STDOUT_FILENO);
                if (statusDupWrite == -1) {
                    errorMessage("Could not redirect the write socket", strerror(errno), serverRessources);
                }
            }

            // close listen connection in the child process
            if (close(serverRessources.fd_socket_listen) != 0) {
                errorMessage("Cloud not close the listen socket in child process", strerror(errno), serverRessources);
            }
            serverRessources.fd_socket_listen = -1;

            // *** Do the exec here ***

            close(serverRessources.fd_socket_connected);
            int status = execl(LOGICS_PATH, LOGICS_NAME, NULL);
            if (status == -1) {
                errorMessage("Could not execute bussiness logic", "error in execl", serverRessources);
                exit(EXIT_FAILURE);
            }

        }
            // ------- PARENT PROCESS --------
        else {
            // Parent Process
            // Close the connected socket in the parent process
            if (verbose == 1) {
                LINEOUTPUT;
                fprintf(stdout, "Parent process, close the connected socket\n");
            }
            close(serverRessources.fd_socket_connected);
            continue;   // next iteration
        }
    }
    return 0;
}

/**
 * @brief signal handler of the forked childs, parent has th wait for the termination and receive the exit code.
 * @param s int: signal
 */
static void sigchild_handler(int s) {
    int save_errno = errno;
    // wait for terminating properly the child process
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = save_errno;
    fprintf(stdout, "signal: %d", s);
}

/**
 * @brief Parameter check for the Server function.
 * @param argc int Number of incoming parameters
 * @param argv char* Pointerarray containing all parametres
 * @param port u_int16_t listening port of the server
 */
static void evaluateParameters(int argc, char* const* argv, u_int16_t* port, int* verbose) {
    int opt;
    char* endpointer = NULL;
    int tempPort = 0;
    /* check the parameters from command line */
    // check if any parameter are given
    if (argc < 2) {
        usage(stderr, argv[0], 1);
    }
    while ((opt = getopt(argc, argv, "hvp:")) != -1) {
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
            case 'v':
                *verbose = 1;
                break;
            default:
                usage(stderr, argv[0], 1);
                break;
        }
    }

}

/**
 * @brief Print out an error message with the given error and terminate the program
 * @param userMessage char* Message to the user
 * @param errorMessage char* errormessage from the system -> equivalent to the set errno
 * @param progname char* name of the programm argv[0]
 */
static void errorMessage(char* userMessage, char* errorMessage, ressources serverRessources) {
    fprintf(stderr, "%s: %s %s\n", serverRessources.progname, userMessage, errorMessage);
    closeRessources(serverRessources);
    exit(EXIT_FAILURE);
}

/**
 * @brief close all open ressources in on single point
 * @param res ressources struct contsining all needed ressources
 */
static void closeRessources(ressources res) {
    if (res.fd_socket_connected != -1) {
        (void) close(res.fd_socket_connected);  // close connected socket
        res.fd_socket_connected = -1;   // set to initialize value
    }
    if (res.fd_socket_listen != -1) {
        (void) close(res.fd_socket_listen); // close listen socket
        res.fd_socket_listen = -1;      // set to initialize value
    }
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
    fprintf(stream, "\t-p <port> \t well-known port of the server [0..65535]\n");
    fprintf(stream, "\t-h \t\t outputs this info\n");
    fprintf(stream, "\t-v\t\t verbose output \n");
    exit(exitcode);
}

/**
 * @brief print the socket address and port to stdout
 * @param sockaddr sockaddr_in: given socket addres
 */
static void printAddress(struct sockaddr_in sockaddr) {
    char address_ip4[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sockaddr.sin_addr, address_ip4, INET_ADDRSTRLEN);
    fprintf(stdout, "%s:%d\n", address_ip4, ntohs(sockaddr.sin_port));

}

/* usage: simple_message_server option
options:
	-p, --port <port>
	-h, --help
*/
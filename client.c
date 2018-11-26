/**
 * @author Valentin Platzgummer - ic17b096
 * @date 18.11.18
 *
 * @brief Implementation of a simplle client Application to work with a server
 * TCP/IP Lecture Distrubted Systems
 */

/* Client needs following syscalls
 * socket()
 * connect()
 * write()
 * read()
 * close()
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

/**
 * @brief is called upon failure and displays a userinfor to stderr and terminates afterwards.
 * @param stream the  stream to write the usage information to (e.g., stdout
                   or stderr).
 * @param cmnd a string containing the name of the executable  (i.e.,  the
                   contents of argv[0]).
 * @param exitcode the  exit code to be used in the call to exit(3) for termi-
                   nating the program.
 */
static void usage(FILE* stream, const char* cmnd, int exitcode);

static void errorMessage(char* userMessage, char* errorMessage, char* progname);

int main(int argc, char* argv[]) {
    int fd_socket;                   // file descriptor client socket
    struct sockaddr_in server_add;   // struct containing server address and port
    char* progname = argv[0];        // Program name for error output
    const char testmessage[] = "This is a test to the server\0";
    ssize_t message_length = strlen(testmessage);
    char receiveBuffer[RECEIVERBUFFER] = {"\0"}; // Receive Buffer
    int i;                      // Counter for message

    // Create a SOCKET(), return value is a file descriptor
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    //
    if (fd_socket < 0) {
        errorMessage("Could not create a socket: ", strerror(errno), progname);
    }
    fprintf(stdout, "Client Socket created.\n");


    // initialize struct with 0
    memset(&server_add, 0, sizeof(server_add));
    /* Convert Internet host address from numbers-and-dots notation in CP
   into binary data in network byte order.  */
    server_add.sin_addr.s_addr = inet_addr(ADDRESS);
    server_add.sin_family = AF_INET;
    server_add.sin_port = htons(PORT);

    // try to CONNECT() to the server
    int success = connect(fd_socket, (struct sockaddr*) &server_add, sizeof(server_add));
    if (success < 0) {
        errorMessage("Could not connect to a Server:", strerror(errno), progname);
    }
    /* inet_ntoa: Convert Internet number in IN to ASCII representation.  The return value
   is a pointer to an internal array containing the string.*/
    fprintf(stdout, "... Connection to Server: %s:%d established\n",
            inet_ntoa(server_add.sin_addr), ntohs(server_add.sin_port));

    /* Here begins the write read loop of the client */
    ssize_t sendbytes, recbytes;
    write(fd_socket, testmessage, sizeof(testmessage));
    if (sendbytes < 0) {
        errorMessage("Could not write to server: ", strerror(errno), progname);
    }

    /* Close the write connection from the client, nothing to say ... */
    if (shutdown(fd_socket, SHUT_WR) < 0) {
        errorMessage("Could not close the WR socket: ", strerror(errno), progname);
    }

    recbytes = read(fd_socket, receiveBuffer, RECEIVERBUFFER);

    if (recbytes < 0) {
        errorMessage("Could not read from server: ", strerror(errno), progname);
    }
    fprintf(stdout, "Server says: %s\n", receiveBuffer);
    /* Close the read connection from the client, over and out ... */
    if (shutdown(fd_socket, SHUT_RDWR) < 0) {
        errorMessage("Could not close the RD socket: ", strerror(errno), progname);
    }
    // print out the received buffer
    fprintf(stdout, "Server says: %\n", receiveBuffer);
    // CLOSE()
    if (close(fd_socket) < 0) {
        errorMessage("Error in closing socket", strerror(errno), progname);
    }
}

/**
 * Print out the error message for th user
 * @param userMessage message appropriate for the user
 * @param errorMessage message from the system
 * @param progname provided from the args
 */
static void errorMessage(char* userMessage, char* errorMessage, char* progname) {
    fprintf(stderr, "%s: %s %s", progname, userMessage, errorMessage);
    exit(EXIT_FAILURE);
}


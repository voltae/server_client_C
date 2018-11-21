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

/**
 * @brief is called upon failure and displays a userinfor to stderr and terminates afterwards.
 * @param stream the  stream to write the usage information to (e.g., stdout
                   or stderr).
 * @param cmnd a string containing the name of the executable  (i.e.,  the
                   contents of argv[0]).
 * @param exitcode the  exit code to be used in the call to exit(3) for termi-
                   nating the program.

 */
void usage(FILE* stream, const char* cmnd, int exitcode);

void errorMessage(char* userMessage, char* errorMessage, char* progname);

int main (int argc, char* argv[]) {
    int fd_socket, connect;
    // progname
    char* progname = argv[0];

    // Create a socket, return value is a file descriptor
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    //
    if (fd_socket < 0) {
        errorMessage("Could not create a socket: ", strerror(errno), progname);
    }

}

    // try to connect to the server

/**
 * Print out the error message for th user
 * @param userMessage message appropriate for the user
 * @param errorMessage message from the system
 * @param progname provided from the args
 */
    void errorMessage(char* userMessage, char* errorMessage, char* progname) {
        fprintf(stderr, "%s: %s %s", progname, userMessage, errorMessage);
    exit(EXIT_FAILURE);
}


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
#include <stdio.h>
#include <stdlib.h>         // provides exit(), EXIT_FAILURE

void usage(char* message, char* progname);
void errorMessage (char *message, char* progname);

int main (int argc, char* argv[]) {
    int fd_socket;
    // progname
    char* progname = argv[0];

    // Create a socket, return value is a file descriptor
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    //
    if (fd_socket < 0)
    {
        errorMessage("Could not create a socket", progname);
    }

}

void errorMessage (char *message, char* progname) {
    fprintf(stderr, "%s %s", progname, message);
    exit(EXIT_FAILURE);
}


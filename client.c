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

#include <stdlib.h>         // provides exit(), EXIT_FAILURE
#include <stdio.h>          // provides the printf()
#include <string.h>         // provide strerror(), strlen()
#include <sys/types.h>
#include <sys/socket.h>     // provides socket
#include <errno.h>          // provides errno
#include <arpa/inet.h>      // provides inet_address
#include <unistd.h>         // provides read(), write(), close()
#include <netdb.h>          // provides getaddrinfo()
// provides smc_parsecommandline()
#include "./libsimple_message_client_commandline_handling/simple_message_client_commandline_handling.h"

/*@TODO Replace testing addresses with user defined from args */
#define ADDRESS "127.0.0.1"
#define PORT 7329
#define RECEIVERBUFFER 100

static void errorMessage(const char* userMessage, const char* errorMessage, const char* progname);

static void usage(FILE* stream, const char* cmnd, int exitcode);

void writeToSocket(int fd_socket, char* message, const char* progname);

int main(int argc, const char* argv[]) {
    int fd_socket, fd_copy;                   // file descriptor client socket
    const char* progname = argv[0];        // Program name for error output
    struct addrinfo* serveraddr, * currentAddr;    // Returnvalue of getaddrinfo(), currentAddr used for loop
    struct addrinfo hints;                   // Hints struct for the addr info function
    //   char remotehost[INET6_ADDRSTRLEN];       // char holding the remote host with 46 len
    const char testmessage[] = "This is a test to the server\0";
    char receiveBuffer[RECEIVERBUFFER] = {"\0"}; // Receive Buffer

    // Declare variables for the line parser
    const char* server = NULL;
    const char* port = NULL;
    const char* user = NULL;
    const char* message = NULL;
    const char* img_url = NULL;
    int verbose = 0;
    // call the argument parser
    smc_parsecommandline(argc, argv, usage, &server, &port, &user, &message, &img_url, &verbose);

    fprintf(stdout, "server: %s, port: %s, message: %s, image_url: %s\n", server, port, message, img_url);

    fprintf(stdout, "Client Socket created.\n");

    // Get the type of connection
    memset(&hints, 0, sizeof(hints));   // set to NULL
    hints.ai_family = AF_UNSPEC;          // Allows IP6 and IP6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;

    int retValue = 0;
    //TODO: hier sollte eigentlich server rein, aber geht nicth warum? NULL geht.
    if ((retValue = getaddrinfo(NULL, port, &hints, &serveraddr) != 0)) {
        errorMessage("Could not resolve hostname.", gai_strerror(retValue), progname);
    }
    // use the struct coming from getaddrinfo

    // looping though the linked list to find a working address
    for (currentAddr = serveraddr; currentAddr != NULL; currentAddr = currentAddr->ai_next) {
        // Create a SOCKET(), return value is a file descriptor
        fd_socket = socket(currentAddr->ai_family, currentAddr->ai_socktype, currentAddr->ai_protocol);
        //
        if (fd_socket < 0) {
            fprintf(stderr, "Could not create a socket:\n");
            continue; // try next pointer
        }

        // try to CONNECT() to the server
        int success = connect(fd_socket, currentAddr->ai_addr, currentAddr->ai_addrlen);
        if (success == -1) {
            fprintf(stderr, "Could not connect to a Server: %s\n", gai_strerror(success));
            close(fd_socket);  // connection failed, close socket.
            continue;   // try next pointer
        }
        break; // connection established
    }
    // if the currentPointer is Null at this point, something was going wrong
    if (currentAddr == NULL) {
        freeaddrinfo(serveraddr);   // Free the allocated pointer before quitting
        errorMessage("Connection failed.", strerror(errno), progname);
    }
    freeaddrinfo(serveraddr);   // free the allocated pointer


    /* inet_ntop: Convert Internet number in IN to ASCII representation.  The return value
   is a pointer to an internal array containing the string.*/
    fprintf(stdout, "... Connection to Server: established\n");



    /* Here begins the write read loop of the client */

    /* using filepointer instead of write */
    char messageFilePointer[] = "This message is going to be send via Filepointer\0";
    /* convert the file descriptor to a File Pointer */
    if ((fd_copy = dup(fd_socket) == -1)) {
        errorMessage("Error in duplicating file descriptor", strerror(errno), progname);
    }

    FILE* client_read_fp, * client_write_fp;
    client_read_fp = fdopen(fd_socket, "r");
    client_write_fp = fdopen(fd_copy, "w");
    if (client_read_fp == NULL || client_write_fp == NULL) {
        // an error occured during cerate Filepointer, close the file descriptor
        close(fd_socket);
        errorMessage("Could not create a File Pointer", strerror(errno), progname);
    }

    int success;
    success = fputs(messageFilePointer, client_write_fp);
    if (success == EOF) {
        // an error occured during write, close the file descriptor
        close(fd_socket);
        errorMessage("Could not write to the File Pointer", strerror(errno), progname);
    }

    ssize_t sendbytes, recbytes;
    sendbytes = write(fd_socket, testmessage, sizeof(testmessage));
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
    fprintf(stdout, "Server says: %s\n", receiveBuffer);
    // CLOSE()
    if (close(fd_socket) < 0) {
        errorMessage("Error in closing socket", strerror(errno), progname);
    }
}

static void errorMessage(const char* userMessage, const char* errorMessage, const char* progname) {
    fprintf(stderr, "%s: %s %s", progname, userMessage, errorMessage);
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
    fprintf(stream, "usage: %s options\n", cmnd);
    fprintf(stream, "options:\n");
    fprintf(stream, "\t-s, --server <server>   full qualified domain name or IP address of the server\n");
    fprintf(stream, "\t-p, --port <port>       well-known port of the server [0..65535]\n");
    fprintf(stream, "\t-u, --user <name>       name of the posting user\n");
    fprintf(stream, "\t-i, --image <URL>       URL pointing to an image of the posting user\n");
    fprintf(stream, "\t-m, --message <message> message to be added to the bulletin board\n");
    fprintf(stream, "\t-v, --verbose           verbose output\n");
    fprintf(stream, "\t-h, --help\n");

    exit(exitcode);
}


/* usage: simple_message_client options
options:
	-s, --server <server>   full qualified domain name or IP address of the server
	-p, --port <port>       well-known port of the server [0..65535]
	-u, --user <name>       name of the posting user
	-i, --image <URL>       URL pointing to an image of the posting user
	-m, --message <message> message to be added to the bulletin board
	-v, --verbose           verbose output
	-h, --help
*/

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
#include <simple_message_client_commandline_handling.h> // provides smc_parsecommandline()
#include <math.h>           // provides floor()


#define RECEIVERBUFFER 100
#define STATUSLENGTH 10
#define MAXFILENAMELENGTH 255
#define MAXFILELENGTH 10        // i.e 10^10 Bytes far enough
#define CHUNK 256

static void errorMessage(const char* userMessage, const char* errorMessage, const char* progname);

static void usage(FILE* stream, const char* cmnd, int exitcode);

//static void writeToSocket(int fd_socket, char* message, const char* progname);

static void writeToDisk(FILE* disk_write_fp, FILE* client_read_fp, int length, const char* progname);

static size_t extractIntfromString(char* buffer, int len);

static char* extractFilename(char* filenameBuffer);


int main(int argc, const char* argv[]) {
    int fd_socket, fd_copy;                     // file descriptor client socket
    const char* progname = argv[0];             // Program name for error output
    struct addrinfo* serveraddr, * currentAddr;    // Returnvalue of getaddrinfo(), currentAddr used for loop
    struct addrinfo hints;                      // Hints struct for the addr info function
    //   char remotehost[INET6_ADDRSTRLEN];       // char holding the remote host with 46 len
    char filenameBuffer[MAXFILENAMELENGTH]; // Buffer for File name max 255 Chars
    char html_lenghtBuffer[MAXFILELENGTH];                  // Max size of length
    int html_fileLength;
    char statusBuffer[STATUSLENGTH];                  // Buffer for the Status
    int status;                                 // integer holds status

    // Declare variables for the line parser
    const char* serverIP = NULL;
    const char* serverPort = NULL;
    const char* user = NULL;
    const char* messageOut = NULL;
    const char* img_url = NULL;
    int verbose = 0;
    // call the argument parser
    smc_parsecommandline(argc, argv, usage, &serverIP, &serverPort, &user, &messageOut, &img_url, &verbose);

    fprintf(stdout, "serverIP: %s, serverPort: %s, messageOut: %s, image_url: %s\n", serverIP, serverPort, messageOut,
            img_url);

    // Get the type of connection Hint struct!
    memset(&hints, 0, sizeof(hints));   // set to NULL
    hints.ai_family = AF_UNSPEC;          // Allows IP4 and IP6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;              // redundand da eh null von vorher
    hints.ai_flags = 0;

    int retValue = 0;
    if ((retValue = getaddrinfo(serverIP, serverPort, &hints, &serveraddr) != 0)) {
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
        fprintf(stdout, "Client Socket created.\n");
        // try to CONNECT() to the serverIP
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
    /* convert the file descriptor to a File Pointer */
    if ((fd_copy = dup(fd_socket)) == -1) {
        errorMessage("Error in duplicating file descriptor", strerror(errno), progname);
    }
    fprintf(stdout, "Copied the socked descr.: orig:%d copy: %d\n", fd_socket, fd_copy);

    FILE* client_read_fp, * client_write_fp, * disk_write_fp;
    client_read_fp = fdopen(fd_copy, "r");      // use the copy of the duplicated field descriptor for both file Pointer
    client_write_fp = fdopen(fd_socket, "w");
    if (client_read_fp == NULL || client_write_fp == NULL) {
        // an error occured during cerate Filepointer, close the file descriptor
        close(fd_socket);
        errorMessage("Could not create a File Pointer", strerror(errno), progname);
    }

    ssize_t sendbytes; //recbytes;
    if (img_url == NULL) {
        sendbytes = fprintf(client_write_fp, "user=%s\n%s", user, messageOut);
    } else {
        sendbytes = fprintf(client_write_fp, "user=%s\nimg=%s\n%s", user, img_url, messageOut);
    }
    // Outcommented we work with the filepointer and fprintf
    //sendbytes = write(fd_socket, messageOut, sizeof(messageOut));
    if (sendbytes < 0) {
        errorMessage("Could not write to serverIP: ", strerror(errno), progname);
    }
    if (sendbytes == -1) {
        // an error occured during write, close the file descriptor
        close(fd_socket);
        errorMessage("Could not write to the File Pointer", strerror(errno), progname);
    } else if (sendbytes == 0) {
        errorMessage("Nothing sended.", strerror(errno), progname);
    }
    fflush(client_write_fp);        //@TODO  ERROR Checking


    /* Close the write connection from the client, nothing to say ... */
    if ((shutdown(fileno(client_write_fp), SHUT_WR)) < 0) {
        errorMessage("Could not close the WR socket: ", strerror(errno), progname);
    } // outcommented for testing

    fclose(client_write_fp);

    // @TODO: close ressources in error case.
    // Get status
    if (fgets(statusBuffer, STATUSLENGTH, client_read_fp) == NULL) {     // fgets uses read descriptor
        errorMessage("Could not read from server: ", strerror(errno), progname);
    }

    status = extractIntfromString(statusBuffer, strlen(statusBuffer));
    fprintf(stdout, "Server says: %s\n", statusBuffer);
    fprintf(stdout, "Status is: %d\n", status);

    // get filename
    if (fgets(filenameBuffer, MAXFILENAMELENGTH, client_read_fp) == NULL) {    // fgets uses read descriptor
        errorMessage("Could not read from server: ", strerror(errno), progname);
    }

    if (fgets(html_lenghtBuffer, MAXFILELENGTH, client_read_fp) == NULL) {
        errorMessage("Could not read from server: ", strerror(errno), progname);
    }
    html_fileLength = extractIntfromString(html_lenghtBuffer, MAXFILELENGTH);

    fprintf(stdout, "Server says: %s\n", filenameBuffer);
    fprintf(stdout, "Server says: %s, %d\n", html_lenghtBuffer, html_fileLength);

    // extract the filename from the field
    char* filename = extractFilename(filenameBuffer);
    fprintf(stderr, "Filename: %s", filename);

    // open filepointer for disk
    disk_write_fp = fopen(filename, "w");

    // we don't need the pointer anymore free it.
    free(filename);

    /* Call the write function */
    writeToDisk(disk_write_fp, client_read_fp, html_fileLength, progname);

    // get filename
    if (fgets(filenameBuffer, MAXFILENAMELENGTH, client_read_fp) == NULL) {    // fgets uses read descriptor
        errorMessage("Could not read from serverIP: ", strerror(errno), progname);
    }
    if (fgets(html_lenghtBuffer, MAXFILELENGTH, client_read_fp) == NULL) {
        errorMessage("Could not read from serverIP: ", strerror(errno), progname);
    }
    int binary_filelenght = extractIntfromString(html_lenghtBuffer, MAXFILELENGTH);

    fprintf(stdout, "Server says: %s\n", filenameBuffer);
    fprintf(stdout, "Server says: %s, %d\n", html_lenghtBuffer, binary_filelenght);

    // extract the filename from the field
    filename = extractFilename(filenameBuffer);
    fprintf(stderr, "Filename: %s", filename);
    FILE* binary = fopen(filename, "w");

    /* Call the write function */
    writeToDisk(binary, client_read_fp, binary_filelenght, progname);

    /* Close the read connection from the client, over and out ... */
    if (shutdown(fileno(client_read_fp), SHUT_RDWR) < 0) {
        errorMessage("Could not close the RD socket: ", strerror(errno), progname);
    }

    // CLOSE() - The read end
    if (close(fileno(client_read_fp)) < 0) {
        errorMessage("Error in closing socket", strerror(errno), progname);
    }
}

void writeToDisk(FILE* disk_write_fp, FILE* client_read_fp, int length, const char* progname) {

    // Create Buffer for the html_text
    char html_text_buffer[length];
    memset(html_text_buffer, 0, sizeof(html_text_buffer));       // write 0 in the buffer

    // create partioned textbuffer for continuous reading
    char* partioned_read_array = malloc(CHUNK);

    // integer declaration for the read and write porcess
    int readBytes = 0, writeBytes = 0, cycles;
    // these are the returnvalues of each systemcall
    int actualRead, actualWrite;
    // calculate howm many partitions we need
    cycles = floor(length / CHUNK);

    // read the file chunk wise in and write those chunks to disk
    for (int i = 0; i < cycles; i++) {
        readBytes += fread(partioned_read_array, 1, CHUNK, client_read_fp);
        writeBytes += fwrite(partioned_read_array, 1, CHUNK, disk_write_fp);
        fprintf(stdout, "Server says: %s\n", partioned_read_array);
        fprintf(stdout, "Client says: read bytes %d\n", readBytes);
        fprintf(stdout, "Client says: written bytes %d\n", writeBytes);
        fprintf(stdout, "Cycle: %d of %d\n", i + 1, cycles);
    }
    free(partioned_read_array); // Done with the partitions - free it

    /* Handle the rest */
    int rest = length - readBytes;
    fprintf(stdout, "Rest to copy %d\n", rest);

    // read the rest if needed.
    if (rest > 0) {
        char* restOfFile = malloc(rest);
        actualRead = fread(restOfFile, 1, rest, client_read_fp);
        if (actualRead == 0) {
            errorMessage("Could not read from server: ", strerror(errno), progname);
        }
        readBytes += actualRead;

        actualWrite = fprintf(disk_write_fp, "%s", restOfFile);
        if (actualWrite == 0) {
            fclose(disk_write_fp);
            errorMessage("Could not write to Disk: ", strerror(errno), progname);
        }
        writeBytes += actualWrite;
        fprintf(stdout, "Server says: %s\n", restOfFile);
        free(restOfFile);   // done with the rest
    }

    fflush(disk_write_fp);
    fclose(disk_write_fp);  // close the file

    fprintf(stdout, "Server says: %s\n", html_text_buffer);
    fprintf(stdout, "Server says: read bytes %d\n", readBytes);
    fprintf(stdout, "Server says: written bytes %d\n", writeBytes);
}

static void errorMessage(const char* userMessage, const char* errorMessage, const char* progname) {
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

static size_t extractIntfromString(char* buffer, int len) {
    char tempBuffer[len];
    size_t resultLen = 0, result;
    for (int i = 0; i < len; i++) {
        if (buffer[i] >= '0' && buffer[i] <= '9')
            tempBuffer[resultLen++] = buffer[i];
    }

    result = (size_t) strtol(tempBuffer, NULL, 10);
    return result;
}

static char* extractFilename(char* filenameBuffer) {
    int a = 0, b = 0, c = 0;
    // find the '=' sign
    while (filenameBuffer[a]) {
        if (filenameBuffer[a++] == '=')
            break;
    }
    // find the '\0'
    b = a;
    while (filenameBuffer[b++]);
    // create a new array
    char* filename = malloc(b - a + 1);
    //copy array
    while (filenameBuffer[a]) {
        filename[c++] = filenameBuffer[a++];
    }
    filename[c] = '\0';     // 0 - Termination of the filename

    return filename;
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

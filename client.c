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
#include <stdbool.h>        // provides true, false


#define RECEIVERBUFFER 100
#define STATUSLENGTH 10
#define MAXFILENAMELENGTH 255
#define MAXFILELENGTH 10        // i.e 10^10 Bytes far enough
#define CHUNK 256

/**
 * All ressources are stored here
 */
typedef struct ressourcesContainer {
    FILE* client_read_fp;
    FILE* client_write_fp;
    FILE* client_write_disk_fp;
    int fd_read_sock;
    int fd_write_sock;
    const char* progname;
} ressourcesContainer;

static void errorMessage(const char* userMessage, const char* errorMessage, ressourcesContainer* ressources);

static void usage(FILE* stream, const char* cmnd, int exitcode);

//static void writeToSocket(int fd_socket, char* message, const char* progname);

static void writeToDisk(int length, ressourcesContainer* ressources);

static int extractIntfromString(char* buffer, int len);

static void extractFilename(char* filenameBuffer, char** filename, ressourcesContainer* ressources);

static void closeAllRessources(ressourcesContainer* ressources);

int main(int argc, const char* argv[]) {
    struct addrinfo* serveraddr, * currentAddr;    // Returnvalue of getaddrinfo(), currentAddr used for loop
    struct addrinfo hints;                      // Hints struct for the addr info function
    //   char remotehost[INET6_ADDRSTRLEN];       // char holding the remote host with 46 len
    char filenameBuffer[MAXFILENAMELENGTH]; // Buffer for File name max 255 Chars
    char html_lenghtBuffer[MAXFILELENGTH];                  // Max size of length
    int html_fileLength;
    char statusBuffer[STATUSLENGTH];                  // Buffer for the Status
    int status;                                 // integer holds status

    // alloocate the ressources struct
    //@TODO: deallocate struct
    ressourcesContainer* ressources = malloc(sizeof(ressourcesContainer));
    if (ressources == NULL) {
        fprintf(stderr, "%s: Could not allocate memory: %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    // set al values to default;
    ressources->client_read_fp = NULL;
    ressources->client_write_fp = NULL;
    ressources->client_write_disk_fp = NULL;
    ressources->fd_read_sock = -1;
    ressources->fd_write_sock = -1;
    ressources->progname = argv[0];


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
        errorMessage("Could not resolve hostname.", gai_strerror(retValue), ressources);
    }
    // use the struct coming from getaddrinfo

    // looping though the linked list to find a working address
    for (currentAddr = serveraddr; currentAddr != NULL; currentAddr = currentAddr->ai_next) {
        // Create a SOCKET(), return value is a file descriptor
        ressources->fd_write_sock = socket(currentAddr->ai_family, currentAddr->ai_socktype, currentAddr->ai_protocol);
        //
        if (ressources->fd_write_sock < 0) {
            fprintf(stderr, "Could not create a socket:\n");
            continue; // try next pointer
        }
        fprintf(stdout, "Client Socket created.\n");
        // try to CONNECT() to the serverIP
        int success = connect(ressources->fd_write_sock, currentAddr->ai_addr, currentAddr->ai_addrlen);
        if (success == -1) {
            fprintf(stderr, "Could not connect to a Server: %s\n", gai_strerror(success));
            close(ressources->fd_write_sock);  // connection failed, close socket.
            continue;   // try next pointer
        }
        break; // connection established
    }
    // if the currentPointer is Null at this point, something was going wrong
    if (currentAddr == NULL) {
        freeaddrinfo(serveraddr);   // Free the allocated pointer before quitting
        closeAllRessources(ressources);
        errorMessage("Connection failed.", strerror(errno), ressources);
    }
    freeaddrinfo(serveraddr);   // free the allocated pointer


    /* inet_ntop: Convert Internet number in IN to ASCII representation.  The return value
   is a pointer to an internal array containing the string.*/
    fprintf(stdout, "... Connection to Server: established\n");

    /* Here begins the write read loop of the client */

    /* using filepointer instead of write */
    /* convert the file descriptor to a File Pointer */
    if ((ressources->fd_read_sock = dup(ressources->fd_write_sock)) == -1) {
        closeAllRessources(ressources);
        errorMessage("Error in duplicating file descriptor", strerror(errno), ressources);
    }
    fprintf(stdout, "Copied the socked descr.: orig:%d copy: %d\n", ressources->fd_write_sock,
            ressources->fd_read_sock);

    ressources->client_read_fp = fdopen(ressources->fd_read_sock,
                                        "r");      // use the copy of the duplicated field descriptor for both file Pointer
    ressources->client_write_fp = fdopen(ressources->fd_write_sock, "w");
    if (ressources->client_read_fp == NULL || ressources->client_write_fp == NULL) {
        // an error occured during cerate Filepointer, close the file descriptor
        closeAllRessources(ressources);
        errorMessage("Could not create a File Pointer", strerror(errno), ressources);
    }

    ssize_t sendbytes; //recbytes;
    if (img_url == NULL) {
        sendbytes = fprintf(ressources->client_write_fp, "user=%s\n%s", user, messageOut);
    } else {
        sendbytes = fprintf(ressources->client_write_fp, "user=%s\nimg=%s\n%s", user, img_url, messageOut);
    }
    // Outcommented we work with the filepointer and fprintf
    //sendbytes = write(fd_socket, messageOut, sizeof(messageOut));
    if (sendbytes < 0) {
        closeAllRessources(ressources);
        errorMessage("Could not write to serverIP: ", strerror(errno), ressources);
    }
    if (sendbytes == -1) {
        // an error occured during write, close the file descriptor
        closeAllRessources(ressources);
        errorMessage("Could not write to the File Pointer", strerror(errno), ressources);
    } else if (sendbytes == 0) {
        closeAllRessources(ressources);
        errorMessage("Nothing sended.", strerror(errno), ressources);
    }
    fflush(ressources->client_write_fp);        //@TODO  ERROR Checking


    /* Close the write connection from the client, nothing to say ... */
    if ((shutdown(fileno(ressources->client_write_fp), SHUT_WR)) < 0) {
        closeAllRessources(ressources);
        errorMessage("Could not close the WR socket: ", strerror(errno), ressources);
    } // outcommented for testing

    fclose(ressources->client_write_fp);
    ressources->client_write_fp = NULL;

    // @TODO: close ressources in error case.
    // Get status
    if (fgets(statusBuffer, STATUSLENGTH, ressources->client_read_fp) == NULL) {     // fgets uses read descriptor
        closeAllRessources(ressources);
        errorMessage("Could not read from server: ", strerror(errno), ressources);
    }

    status = extractIntfromString(statusBuffer, strlen(statusBuffer));
    fprintf(stdout, "Server says: %s\n", statusBuffer);
    fprintf(stdout, "Status is: %d\n", status);

    // get filename
    if (fgets(filenameBuffer, MAXFILENAMELENGTH, ressources->client_read_fp) == NULL) {    // fgets uses read descriptor
        closeAllRessources(ressources);
        errorMessage("Could not read from server: ", strerror(errno), ressources);
    }

    if (fgets(html_lenghtBuffer, MAXFILELENGTH, ressources->client_read_fp) == NULL) {
        closeAllRessources(ressources);
        errorMessage("Could not read from server: ", strerror(errno), ressources);
    }
    html_fileLength = extractIntfromString(html_lenghtBuffer, MAXFILELENGTH);

    fprintf(stdout, "Server says: %s\n", filenameBuffer);
    fprintf(stdout, "Server says: %s, %d\n", html_lenghtBuffer, html_fileLength);

    // extract the filename from the field
    char* filename = NULL;
    extractFilename(filenameBuffer, &filename, ressources);
    fprintf(stderr, "Filename: %s", filename);

    // open filepointer for disk
    ressources->client_write_disk_fp = fopen(filename, "w");

    // we don't need the pointer anymore free it.
    free(filename);
    filename = NULL;

    /* Call the write function */
    writeToDisk(html_fileLength, ressources);

    // get filename
    if (fgets(filenameBuffer, MAXFILENAMELENGTH, ressources->client_read_fp) == NULL) {    // fgets uses read descriptor
        closeAllRessources(ressources);
        errorMessage("Could not read from serverIP: ", strerror(errno), ressources);
    }
    if (fgets(html_lenghtBuffer, MAXFILELENGTH, ressources->client_read_fp) == NULL) {
        closeAllRessources(ressources);
        errorMessage("Could not read from serverIP: ", strerror(errno), ressources);
    }
    int binary_filelenght = extractIntfromString(html_lenghtBuffer, MAXFILELENGTH);

    fprintf(stdout, "Server says: %s\n", filenameBuffer);
    fprintf(stdout, "Server says: %s, %d\n", html_lenghtBuffer, binary_filelenght);

    // extract the filename from the field
    extractFilename(filenameBuffer, &filename, ressources);
    fprintf(stderr, "Filename: %s", filename);
    ressources->client_write_disk_fp = fopen(filename, "w");

    // we don't need the pointer anymore free it.
    free(filename);
    filename = NULL;

    /* Call the write function */
    writeToDisk(binary_filelenght, ressources);

    /* Close the read connection from the client, over and out ... */
    int lastshutdown;
    if ((lastshutdown = shutdown(fileno(ressources->client_read_fp), SHUT_RDWR)) == -1) {
        closeAllRessources(ressources);
        errorMessage("Could not close the RD socket: ", strerror(errno), ressources);
    }
    printf("Last shutdown exit code: %d\n", lastshutdown);

    // CLOSE() - The read end
    if (close(fileno(ressources->client_read_fp)) < 0) {
        closeAllRessources(ressources);
        errorMessage("Error in closing socket", strerror(errno), ressources);
    }
    // close the filestream
    fclose(ressources->client_read_fp);
    ressources->client_read_fp = NULL;

    // Everything went well, deallocate the ressources struct
    free(ressources);
}

void writeToDisk(int length, ressourcesContainer* ressources) {

    // Create Buffer for the html_text
    char html_text_buffer[length];
    memset(html_text_buffer, 0, sizeof(html_text_buffer));       // write 0 in the buffer

    // create partioned textbuffer for continuous reading

    char* partioned_read_array = malloc(CHUNK);
    if (partioned_read_array == NULL) {
        closeAllRessources(ressources);
        errorMessage("Could not allocate memory for buffer.", strerror(errno), ressources);
    }
    // integer declaration for the read and write porcess
    int readBytes = 0, writeBytes = 0, cycles;
    // these are the returnvalues of each systemcall
    int actualRead, actualWrite;
    // calculate howm many partitions we need
    cycles = floor(length / CHUNK);

    // read the file chunk wise in and write those chunks to disk
    // @TODO: check for EOF
    bool isEOF = false;

    for (int i = 0; i < cycles && isEOF == false; i++) {
        readBytes += fread(partioned_read_array, 1, CHUNK, ressources->client_read_fp);
        // check if EOF
        if (feof(ressources->client_read_fp) != 0) {
            isEOF = true;
        }
        if (ferror(ressources->client_read_fp) != 0) {
            closeAllRessources(ressources);
            errorMessage("Error in reading from socket", strerror(errno), ressources);
        }
        writeBytes += fwrite(partioned_read_array, 1, CHUNK, ressources->client_write_disk_fp);
        fprintf(stdout, "Server says: %s\n", partioned_read_array);
        fprintf(stdout, "Client says: read bytes %d\n", readBytes);
        fprintf(stdout, "Client says: written bytes %d\n", writeBytes);
        fprintf(stdout, "Cycle: %d of %d\n", i + 1, cycles);
    }
    free(partioned_read_array); // Done with the partitions - free it

    /* Handle the rest , only when EOF is not set*/
    if (isEOF == false) {
        int rest = length - readBytes;
        fprintf(stdout, "Rest to copy %d\n", rest);

        // read the rest if needed.
        if (rest > 0) {
            char* restOfFile = malloc(rest);
            if (restOfFile == NULL) {
                closeAllRessources(ressources);
                errorMessage("Could not allocate memory for the filebuffer", strerror(errno), ressources);
            }
            actualRead = fread(restOfFile, 1, rest, ressources->client_read_fp);
            if (actualRead == 0) {
                errorMessage("Could not read from server: ", strerror(errno), ressources);
            }
            // check if EOF
            if (feof(ressources->client_read_fp) != 0) {
                isEOF = true;
            }
            if (ferror(ressources->client_read_fp) != 0) {
                closeAllRessources(ressources);
                errorMessage("Error in reading from socket", strerror(errno), ressources);
            }

            readBytes += actualRead;

            actualWrite = fprintf(ressources->client_write_disk_fp, "%s", restOfFile);
            if (actualWrite == 0) {
                closeAllRessources(ressources);
                errorMessage("Could not write to Disk: ", strerror(errno), ressources);
            }
            writeBytes += actualWrite;
            fprintf(stdout, "Server says: %s\n", restOfFile);
            free(restOfFile);   // done with the rest
            restOfFile = NULL;
        }
    }

    fflush(ressources->client_write_disk_fp);
    fclose(ressources->client_write_disk_fp);  // close the file
    ressources->client_write_disk_fp = NULL;

    fprintf(stdout, "Server says: %s\n", html_text_buffer);
    fprintf(stdout, "Server says: read bytes %d\n", readBytes);
    fprintf(stdout, "Server says: written bytes %d\n", writeBytes);
}

static void errorMessage(const char* userMessage, const char* errorMessage, ressourcesContainer* ressources) {
    fprintf(stderr, "%s: %s %s\n", ressources->progname, userMessage, errorMessage);
    free(ressources);       // deallocate the ressources before leaving program
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

static int extractIntfromString(char* buffer, int len) {
    char tempBuffer[len];
    int resultLen = 0, result;
    for (int i = 0; i < len; i++) {
        if (buffer[i] >= '0' && buffer[i] <= '9')
            tempBuffer[resultLen++] = buffer[i];
    }
    // @TODO: return -1 when eror and handle it in caller function
    result = (int) strtol(tempBuffer, NULL, 10);
    return result;
}

//
static void extractFilename(char* filenameBuffer, char** filename, ressourcesContainer* ressources) {
    int a = 0, b = 0, c = 0;
    // find the '=' sign
    while (filenameBuffer[a]) {
        if (filenameBuffer[a++] == '=') { break; }
    }
    // find the '\0'
    b = a;
    while (filenameBuffer[b++]); // find endline '\0'
    // Filename is too long, pritn out an error
    if (b >= MAXFILENAMELENGTH) {
        closeAllRessources(ressources);
        errorMessage("Filename is to long", "Buffer overflow", ressources);
    }
    char* filenameTemp = NULL;
    // create a new array
    filenameTemp = malloc((b - a) * sizeof(char));
    if (filenameTemp == NULL) {
        // @TODO: Close all open ressources
        fprintf(stderr, "error allocation memory");
    }
    //copy array
    while (filenameBuffer[a]) {
        filenameTemp[c++] = filenameBuffer[a++];
    }
    // Termination of the filename, delete the '\n' one char before 0.
    filenameTemp[c - 1] = '\0';
    *filename = filenameTemp;
}

static void closeAllRessources(ressourcesContainer* ressources) {
    if (ressources->fd_read_sock != -1) {
        close(ressources->fd_read_sock);
        ressources->fd_read_sock = -1;
    }
    if (ressources->fd_write_sock != -1) {
        close(ressources->fd_write_sock);
        ressources->fd_write_sock = -1;
    }
    if (ressources->client_read_fp != NULL) {
        fclose(ressources->client_read_fp);
        ressources->client_read_fp = NULL;
    }
    if (ressources->client_write_fp != NULL) {
        fclose(ressources->client_write_fp);
        ressources->client_write_fp = NULL;
    }
    if (ressources->client_write_disk_fp != NULL) {
        fclose(ressources->client_write_disk_fp);
        ressources->client_write_disk_fp = NULL;
    }

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

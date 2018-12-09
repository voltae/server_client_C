/**
 * @author Valentin Platzgummer - ic17b096
 * @author Lara Kammerer - ic17b001
 * @date 18.11.18
 * @brief Implementation of a simple client application to communicate with a server application
 * TCP/IP Lecture Distrubted Systems
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


/** @def length of the field status max 10 */
#define STATUSLENGTH 10
/** @def length of the field filename max 255 bytes */
#define MAXFILENAMELENGTH 255
/** @def length of the field file length max 10 bytes i.e 10^10 Bytes far enough */
#define MAXFILELENGTH 10
/** @def chunck size of the reading buffer  */
#define CHUNK 256

#define LINEOUTPUT fprintf(stdout, "[%s, %s, %d]: ",  __FILE__, __func__, __LINE__)
/**
 * @brief ressourcesContainer stores all needed ressources in one single place
 */
typedef struct ressourcesContainer {
    FILE* filepointerClientRead;             /**< File Pointer for Read operation */
    FILE* filepointerClientWrite;            /**< File Pointer for Write operation */
    FILE* clientWriteDiskFp;                 /**< File Pointer for Hard Disk operation */
    int socketDescriptorRead;                /**< Socket for Read operation */
    int socketDescriptorWrite;               /**< Socket for Write operation */
    const char* progname;                    /**< Program Name argv[0] */
    int verbose;                             /**< Output in verbose mode 0 off, 1 on */
} ressourcesContainer;

static void errorMessage(const char* userMessage, const char* errorMessage, ressourcesContainer* ressources);

static void usage(FILE* stream, const char* cmnd, int exitcode);

static void printAddress(struct sockaddr* sockaddr);

static bool writeToDisk(int length, ressourcesContainer* ressources);

static int extractIntfromString(char* buffer, int len);

static void extractFilename(char* filenameBuffer, char** filename, ressourcesContainer* ressources);

static void closeAllRessources(ressourcesContainer* ressources);

int main(int argc, const char* argv[]) {
    struct addrinfo* serveraddr, * currentServerAddr;    // Returnvalue of getaddrinfo(), currentAddr used for loop
    struct addrinfo hints;                      // Hints struct for the addr info function
    char statusBuffer[STATUSLENGTH];                  // Buffer for the Status
    int extractedStatus;                                 // integer holds status
    char filenameBuffer[MAXFILENAMELENGTH]; // Buffer for File name max 255 Chars
    char htmlLenghtBuffer[MAXFILELENGTH];                  // Max size of length
    int extractedHtmlFileLength;

    //--------------------------------------------------
    //----------allocate the ressources struct----------
    //--------------------------------------------------
    //@TODO: deallocate struct
    ressourcesContainer* ressources = malloc(sizeof(ressourcesContainer));
    if (ressources == NULL) {
        //nicht �ber fkt errorMessage?----------------------------------------------------------------------------------------------------------------------------------
        fprintf(stderr, "%s: Could not allocate memory: %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    //--------------------------------------------
    //----------set al values to default----------
    //--------------------------------------------
    ressources->filepointerClientRead = NULL;
    ressources->filepointerClientWrite = NULL;
    ressources->clientWriteDiskFp = NULL;
    ressources->socketDescriptorRead = -1;
    ressources->socketDescriptorWrite = -1;
    ressources->progname = argv[0];
    ressources->verbose = 0;

    //---------------------------------------------------------
    //----------declare variables for the line parser----------
    //---------------------------------------------------------
    const char* serverIP = NULL;
    const char* serverPort = NULL;
    const char* user = NULL;
    const char* messageOut = NULL;
    const char* imgUrl = NULL;

    // call the argument parser
    smc_parsecommandline(argc, argv, usage, &serverIP, &serverPort, &user, &messageOut, &imgUrl, &ressources->verbose);

    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Using this options: serverIP: %s, serverPort: %s, messageOut: %s, image_url: %s\n",
                serverIP, serverPort, messageOut, imgUrl);
    }

    // Get the type of connection Hint struct!
    memset(&hints, 0, sizeof(hints));   // set to NULL
    hints.ai_family = AF_UNSPEC;          // Allows IP4 and IP6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;              // redundand da eh null von vorher
    hints.ai_flags = 0;
    //----------------------------------------------------
    //----------initialise members of serveraddr----------
    //----------------------------------------------------
    int addrinfoError = 0;
    if ((addrinfoError = getaddrinfo(serverIP, serverPort, &hints, &serveraddr) !=
                         0)) { //getaddrinfo returns 0 if succeeded, works on serveraddr & hints with known serverPort&IP
        //nicht �ber fkt errorMessage?----------------------------------------------------------------------------------------------------------------------------------
        errorMessage("Could not resolve hostname.", gai_strerror(addrinfoError), ressources);
    }
    // use the struct coming from getaddrinfo

    //--------------------------------------------------------------------------
    //----------connect client to serverSocket (socketDescriptorWrite)----------
    //--------------------------------------------------------------------------
    // looping though the linked list to find a working address
    for (currentServerAddr = serveraddr;
         currentServerAddr != NULL; currentServerAddr = currentServerAddr->ai_next) {
        // Create a SOCKET(), return value is a file descriptor
        //socket returns socket descriptor (int), negative value if failed
        ressources->socketDescriptorWrite = socket(currentServerAddr->ai_family, currentServerAddr->ai_socktype,
                                                   currentServerAddr->ai_protocol);
        if (ressources->socketDescriptorWrite < 0) {
            //nicht �ber errorMessage?----------------------------------------------------------------------------------------------------------------------------------
            fprintf(stderr, "Could not create a socket:\n");
            close(ressources->socketDescriptorWrite);
            continue; // try next pointer
        }
        if (ressources->verbose == 1) {
            LINEOUTPUT;
            fprintf(stdout, "Client Socket created.\n");
        }
        // try to CONNECT() to the serverIP
        //connect returns -1 if failed to connect
        int success = connect(ressources->socketDescriptorWrite, currentServerAddr->ai_addr,
                              currentServerAddr->ai_addrlen);
        if (success == -1) {
            //nicht �ber errorMessage?----------------------------------------------------------------------------------------------------------------------------------
            fprintf(stderr, "Could not connect to a Server: %s\n", gai_strerror(success));
            close(ressources->socketDescriptorWrite);  // connection failed, close socket.
            continue;   // try next pointer
        }
        break; // connection established
    }

    // if the currentPointer is Null at this point, something was going wrong
    if (currentServerAddr == NULL) {
        freeaddrinfo(serveraddr);   // Free the allocated pointer before quitting
        errorMessage("Connection failed.", strerror(errno), ressources);
    }
    freeaddrinfo(serveraddr);   // free the allocated pointer


    /* inet_ntop: Convert Internet number in IN to ASCII representation.  The return value
   is a pointer to an internal array containing the string.*/
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, " ... Connection to Server established: ");
        printAddress(serveraddr->ai_addr);
    }

    /* Here begins the write read loop of the client */

    //---------------------------------------------------------------------------------
    //----------establish reading connection to server (socketDescriptorRead)----------
    //---------------------------------------------------------------------------------
    /* using filepointer instead of write */
    /* convert the file descriptor to a File Pointer */
    //dup returns -1 if failed, descriptor if succeeded
    ressources->socketDescriptorRead = dup(ressources->socketDescriptorWrite);
    if (ressources->socketDescriptorRead == -1) {
        errorMessage("Error in duplicating file descriptor", strerror(errno), ressources);
    }
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Copied the socked descr.: orig:%d copy: %d\n",
                ressources->socketDescriptorWrite, ressources->socketDescriptorRead);
    }
    if (ressources->verbose == 1) {
        fprintf(stdout, "[%s, %s, %d]: Creating a write socket\n",
                __FILE__, __func__, __LINE__);
    }
    //---------------------------------------------------------------------------------------------------
    //----------open Filepointer to write/read (filepointerClientRead & filepointerClientWrite)----------
    //---------------------------------------------------------------------------------------------------
    //fdopen returns NULL if failed
    // use the copy of the duplicated field descriptor for both file pointer
    ressources->filepointerClientRead = fdopen(ressources->socketDescriptorRead, "r");
    ressources->filepointerClientWrite = fdopen(ressources->socketDescriptorWrite, "w");

    if (ressources->filepointerClientRead == NULL || ressources->filepointerClientWrite == NULL) {
        // an error occured during cerate Filepointer, close the file descriptor
        errorMessage("Could not create a File Pointer", strerror(errno), ressources);
    }
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Creating filepointer for reading and writing\n");
    }
    ssize_t sentBytes; //recbytes;
    if (imgUrl == NULL) {
        //fprintf returns bytes written to messageOut
        sentBytes = fprintf(ressources->filepointerClientWrite, "user=%s\n%s", user, messageOut);
    } else {
        sentBytes = fprintf(ressources->filepointerClientWrite, "user=%s\nimg=%s\n%s", user, imgUrl, messageOut);
    }
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Send following message to the server. user: %s, image: %s, message: %s\n", user, imgUrl,
                messageOut);
    }
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Send %ld Bytes to the server\n", sentBytes);
    }
    if (sentBytes == -1) {
        // an error occured during write, close the file descriptor
        errorMessage("Could not write to the File Pointer", strerror(errno), ressources);
    } else if (sentBytes == 0) {
        errorMessage("Nothing sent.", strerror(errno), ressources);
    }
    //---------------------------------------------------------------------------------------------------
    //------------------ close Filepointer to write (filepointerClientWrite) ----------------------------
    //---------------------------------------------------------------------------------------------------
    // flush the buffer to socket
    if (fflush(ressources->filepointerClientWrite) != 0) {
        errorMessage("Could not flush to socket", strerror(errno), ressources);
    }

    // Close the write connection from the client, nothing to say ...
    if ((shutdown(fileno(ressources->filepointerClientRead), SHUT_WR) < 0)) {
        errorMessage("Could not shutdown the WR socket of the reading socket: ", strerror(errno), ressources);
    }
    // Close the filepointer closes the underlying socket (socketDescriptorWrite)
    if (fclose(ressources->filepointerClientWrite) != 0) {
        errorMessage("Could not close the write filestream to socket", strerror(errno), ressources);
    }
    ressources->filepointerClientWrite = NULL;
    ressources->socketDescriptorWrite = -1;
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Close Write Filepointer\n");
    }
    // Get status
    if (fgets(statusBuffer, STATUSLENGTH, ressources->filepointerClientRead) ==
        NULL) {     // fgets uses read descriptor
        errorMessage("Could not read from server: ", strerror(errno), ressources);
    }
    // try to find status, the beginning of the protocol

    extractedStatus = extractIntfromString(statusBuffer, strlen(statusBuffer));
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Status is: %d\n", extractedStatus);
    }

    // Loop till finding EOF
    // bool isEOF = false;
    bool isEOF = false;
    do {
        // get filename
        if (fgets(filenameBuffer, MAXFILENAMELENGTH, ressources->filepointerClientRead) == NULL) {
            if (feof(ressources->filepointerClientRead)) {
                isEOF = true;
                if (ressources->verbose == 1) {
                    LINEOUTPUT;
                    fprintf(stdout, "End of File reached: %d\n", isEOF);
                }
                break;
            } else if (ferror(ressources->filepointerClientRead)) {
                closeAllRessources(ressources);
                errorMessage("Could not read from server: ", strerror(errno), ressources);
            }
        }

        if (fgets(htmlLenghtBuffer, MAXFILELENGTH, ressources->filepointerClientRead) == NULL) {
            if (feof(ressources->filepointerClientRead)) {
                isEOF = true;
                if (ressources->verbose == 1) {
                    LINEOUTPUT;
                    fprintf(stdout, "End of File reached: %d\n", isEOF);
                }
                break;
            } else if (ferror(ressources->filepointerClientRead)) {
                closeAllRessources(ressources);
                errorMessage("Could not read from server: ", strerror(errno), ressources);
            }
        }
        extractedHtmlFileLength = extractIntfromString(htmlLenghtBuffer, MAXFILELENGTH);

        if (ressources->verbose == 1) {
            LINEOUTPUT;
            fprintf(stdout, "File length buffer: %s,  length: %d\n",
                    htmlLenghtBuffer, extractedHtmlFileLength);
        }
        // extract the filename from the field
        char* filename = NULL;
        extractFilename(filenameBuffer, &filename, ressources);
        if (ressources->verbose == 1) {
            LINEOUTPUT;
            fprintf(stdout, "Filename: %s\n", filename);
        }
        // open filepointer for disk
        ressources->clientWriteDiskFp = fopen(filename, "w");

        // we don't need the pointer anymore free it.
        free(filename);
        filename = NULL;

        /* Call the write function */
        isEOF = writeToDisk(extractedHtmlFileLength, ressources);
    } while (!isEOF);

    //---------------------------------------------------------------------------------------------------
    //------------------ close Filepointer to read (filepointerClientRead) ------------------------------
    //---------------------------------------------------------------------------------------------------

    // close the read filestream and the underlying socket
    if (fclose(ressources->filepointerClientRead) != 0) {
        errorMessage("Could not close the read filestream to socket", strerror(errno), ressources);
    }
    ressources->filepointerClientRead = NULL;
    ressources->socketDescriptorRead = -1;
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Closing Filestream Client Read \n");
    }
    /* Close the read connection from the client, over and out ... */
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "socketDescriptorRead: %d\n", ressources->socketDescriptorRead);
    }
    // Everything went well, deallocate the ressources struct
    free(ressources);
    return extractedStatus;
}

bool writeToDisk(int length, ressourcesContainer* ressources) {
    static int loops;
    // create partioned textbuffer for continuous reading
    char* partioned_read_array = malloc(CHUNK * sizeof(char));
    memset(partioned_read_array, 0, sizeof(*partioned_read_array));

    // integer declaration for the read and write porcess
    int readBytes = 0, writeBytes = 0, cycles = 0;
    int actualRead = 0, actualWrite = 0;
    // calculate how many partitions we need
    cycles = floor((double) length / CHUNK);

    // read the file chunk wise in and write those chunks to disk
    // @TODO: check for EOF
    bool isEOF = false;
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Begin the read process.\n");
    }

    for (int i = 0; i < cycles && isEOF == false; i++) {
        readBytes += fread(partioned_read_array, 1, CHUNK, ressources->filepointerClientRead);
        // check if EOF
        if (feof(ressources->filepointerClientRead) != 0) {
            isEOF = true;
        }
        if (ferror(ressources->filepointerClientRead) != 0) {
            closeAllRessources(ressources);
            errorMessage("Error in reading from socket", strerror(errno), ressources);
        }
        writeBytes += fwrite(partioned_read_array, 1, CHUNK, ressources->clientWriteDiskFp);
        if (ferror(ressources->clientWriteDiskFp) != 0) {
            closeAllRessources(ressources);
            errorMessage("Error in writing to disk", strerror(errno), ressources);
        }

        if (ressources->verbose == 1) {
            LINEOUTPUT;
            fprintf(stdout, "Read bytes %d from Server\n", readBytes);
            LINEOUTPUT;
            fprintf(stdout, "Written bytes %d to disk\n", writeBytes);
            LINEOUTPUT;
            fprintf(stdout, "Number ofCycle: %d of %d\n", i + 1, cycles);
        }
        if (ressources->verbose == 1) {
            LINEOUTPUT;
            fprintf(stdout, "Wrote to Disk: %s\n", partioned_read_array);
        }
    }

    /* Handle the rest , only when EOF is not set*/
    if (isEOF == false) {
        int lengthRest = length - readBytes;
        if (ressources->verbose == 1) {
            LINEOUTPUT;
            fprintf(stdout, "Rest to copy: %d\n", lengthRest);
        }

        // read the lengthRest if needed.
        if (lengthRest > 0) {
            char* contentRestOfFile = malloc(lengthRest);
            if (contentRestOfFile == NULL) {
                closeAllRessources(ressources);
                errorMessage("Could not allocate memory for the filebuffer", strerror(errno), ressources);
            }
            actualRead = fread(contentRestOfFile, 1, lengthRest, ressources->filepointerClientRead);
            readBytes += actualRead;
            // check if EOF
            if (feof(ressources->filepointerClientRead) != 0) {
                isEOF = true;
            }
            if (ferror(ressources->filepointerClientRead) != 0) {
                closeAllRessources(ressources);
                errorMessage("Error in reading from socket", strerror(errno), ressources);
            }

            actualWrite = fwrite(contentRestOfFile, 1, lengthRest, ressources->clientWriteDiskFp);
            writeBytes += actualWrite;
            if (ferror(ressources->clientWriteDiskFp) != 0) {
                closeAllRessources(ressources);
                errorMessage("Error in writing to disk", strerror(errno), ressources);
            }
            if (ressources->verbose == 1) {
                LINEOUTPUT;
                fprintf(stdout, "Wrote %d of %d read Bytes to disk\n", actualWrite, actualRead);
            }
            if (ressources->verbose == 1) {
                LINEOUTPUT;
                fprintf(stdout, "Wrote to Disk: %s\n", contentRestOfFile);
            }
            free(contentRestOfFile);   // done with the lengthRest
            contentRestOfFile = NULL;
        }
    }

    fflush(ressources->clientWriteDiskFp);
    fclose(ressources->clientWriteDiskFp);  // close the file
    ressources->clientWriteDiskFp = NULL;

    free(partioned_read_array);
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Read bytes %d from Server\n", readBytes);
        LINEOUTPUT;
        fprintf(stdout, "Written bytes %d to disk\n", writeBytes);
    }
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Is end of File: %d\n", isEOF);
    }
    loops++;
    fprintf(stdout, "Looping the write routine: %d\n", loops);


    return isEOF;
}

static void errorMessage(const char* userMessage, const char* errorMessage, ressourcesContainer* ressources) {
    fprintf(stderr, "%s: %s %s\n", ressources->progname, userMessage, errorMessage);
    closeAllRessources(ressources); // close all open ressources before leaving
    if (ressources != NULL) {
        free(ressources);       // deallocate the ressources before leaving program
        ressources = NULL;
    }
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
    errno = 0;
    char tempBuffer[len];
    int resultLen = 0, result;
    for (int i = 0; i < len; i++) {
        if (buffer[i] >= '0' && buffer[i] <= '9')
            tempBuffer[resultLen++] = buffer[i];
    }
    // @TODO: return -1 when eror and handle it in caller function------------------------------------------------------------------------------------------------------
    result = (int) strtol(tempBuffer, NULL, 10);
    if (errno == EINVAL) {
        fprintf(stderr, "Illegal digit");
    }

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
    if (b >= MAXFILENAMELENGTH - 1) {
        closeAllRessources(ressources);
        errorMessage("Filename is to long", "Buffer overflow", ressources);
    }
    char* filenameTemp = NULL;
    // create a new array
    filenameTemp = malloc((b - a) * sizeof(char));
    if (filenameTemp == NULL) {
        errorMessage("error allocation memory", strerror(errno), ressources);
    }
    //copy array
    while (filenameBuffer[a]) {
        filenameTemp[c++] = filenameBuffer[a++];
    }
    // Termination of the filename, delete the '\n' one char before 0.
    filenameTemp[c - 1] = '\0';
    *filename = filenameTemp;
    LINEOUTPUT;
    fprintf(stdout, "Filename:%s\n ", filenameTemp);

}

static void closeAllRessources(ressourcesContainer* ressources) {
    if (ressources->socketDescriptorRead != -1) {
        if (close(ressources->socketDescriptorRead) != 0) {
            fprintf(stderr, "Could not close socketDescriptorRead");
        }
        ressources->socketDescriptorRead = -1;
    }
    if (ressources->socketDescriptorWrite != -1) {
        if (close(ressources->socketDescriptorWrite) != 0) {
            fprintf(stderr, "Could not close socketDescriptorWrite");
        }
        ressources->socketDescriptorWrite = -1;
    }
    if (ressources->filepointerClientRead != NULL) {
        if (fclose(ressources->filepointerClientRead) != 0) {
            fprintf(stderr, "Could not close filepointerClientRead");
        }
        ressources->filepointerClientRead = NULL;
    }
    if (ressources->filepointerClientWrite != NULL) {
        if (fclose(ressources->filepointerClientWrite) != 0) {
            fprintf(stderr, "Could not close filepointerClientWrite");
        }
        ressources->filepointerClientWrite = NULL;
    }
    if (ressources->clientWriteDiskFp != NULL) {
        if (fclose(ressources->clientWriteDiskFp) != 0) {
            fprintf(stderr, "Could not close clientWriteDiskFp");
        }
        ressources->clientWriteDiskFp = NULL;
    }
}

/**
 * @brief print the socket address and port to stdout
 * @param sockaddr sockaddr_in: given socket addres
 */
static void printAddress(struct sockaddr* sockaddr) {
    char address_ip4[INET_ADDRSTRLEN];
    char address_ip6[INET6_ADDRSTRLEN];
    switch (sockaddr->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in*) sockaddr)->sin_addr), address_ip4, INET_ADDRSTRLEN);
            fprintf(stdout, "%s:%d\n", address_ip4, ntohs(((struct sockaddr_in*) sockaddr)->sin_port));
            break;
        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6*) sockaddr)->sin6_addr), address_ip6, INET6_ADDRSTRLEN);
            fprintf(stdout, "%s:%d\n", address_ip6, ntohs(((struct sockaddr_in6*) sockaddr)->sin6_port));
            break;
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

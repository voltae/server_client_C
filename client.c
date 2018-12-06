/**
 * @author Valentin Platzgummer - ic17b096
 * @author Lara Kammerer - ic17b001
 * @date 18.11.18
 *
 * @brief Implementation of a simple client Application to work with a server
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

/**
 * @brief ressourcesContainer stores all needed ressources in one single place
 */
typedef struct ressourcesContainer {
    FILE* filepointerClientRead;           /**< File Pointer for Read operation */
    FILE* filepointerClientWrite;          /**< File Pointer for Write operation */
    FILE* clientWriteDiskFp;     /**< File Pointer for Hard Disk operation */
    int socketDescriptorRead;               /**< Socket for Read operation */
    int socketDescriptorWrite;              /**< Socket for Write operation */
    const char* progname;           /**< Program Name argv[0] */
} ressourcesContainer;

static void errorMessage(const char* userMessage, const char* errorMessage, ressourcesContainer* ressources);

static void usage(FILE* stream, const char* cmnd, int exitcode);

//static void writeToSocket(int fd_socket, char* message, const char* progname);

static void writeToDisk(int length, ressourcesContainer* ressources);

static int extractIntfromString(char* buffer, int len);

static void extractFilename(char* filenameBuffer, char** filename, ressourcesContainer* ressources);

static void closeAllRessources(ressourcesContainer* ressources);

int main(int argc, const char* argv[]) {
    struct addrinfo* serveraddr, * currentServerAddr;    // Returnvalue of getaddrinfo(), currentAddr used for loop
    struct addrinfo hints;                      // Hints struct for the addr info function
    //   char remotehost[INET6_ADDRSTRLEN];       // char holding the remote host with 46 len
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
		//nicht über fkt errorMessage?----------------------------------------------------------------------------------------------------------------------------------
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

	//---------------------------------------------------------
    //----------declare variables for the line parser----------
	//---------------------------------------------------------
    const char* serverIP = NULL;
    const char* serverPort = NULL;
    const char* user = NULL;
    const char* messageOut = NULL;
    const char* imgUrl = NULL;
    int verbose = 0;
    // call the argument parser
    smc_parsecommandline(argc, argv, usage, &serverIP, &serverPort, &user, &messageOut, &imgUrl, &verbose);

    fprintf(stdout, "serverIP: %s, serverPort: %s, messageOut: %s, image_url: %s\n", serverIP, serverPort, messageOut,
            imgUrl);

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
    if ((addrinfoError = getaddrinfo(serverIP, serverPort, &hints, &serveraddr) != 0)) { //getaddrinfo returns 0 if succeeded, works on serveraddr & hints with known serverPort&IP
		//nicht über fkt errorMessage?----------------------------------------------------------------------------------------------------------------------------------
        errorMessage("Could not resolve hostname.", gai_strerror(addrinfoError), ressources);
    }
    // use the struct coming from getaddrinfo

	//--------------------------------------------------------------------------
	//----------connect client to serverSocket (socketDescriptorWrite)----------
	//--------------------------------------------------------------------------
    // looping though the linked list to find a working address
    for (currentServerAddr = serveraddr; currentServerAddr != NULL; currentServerAddr = currentServerAddr->ai_next) {
        // Create a SOCKET(), return value is a file descriptor
		//socket returns socket descriptor (int), negative value if failed
        ressources->socketDescriptorWrite = socket(currentServerAddr->ai_family, currentServerAddr->ai_socktype, currentServerAddr->ai_protocol);
        if (ressources->socketDescriptorWrite < 0) {
			//nicht über errorMessage?----------------------------------------------------------------------------------------------------------------------------------
            fprintf(stderr, "Could not create a socket:\n");
            continue; // try next pointer
        }
        fprintf(stdout, "Client Socket created.\n");
        // try to CONNECT() to the serverIP
		//connect returns -1 if failed to connect
        int success = connect(ressources->socketDescriptorWrite, currentServerAddr->ai_addr, currentServerAddr->ai_addrlen);
        if (success == -1) {
			//nicht über errorMessage?----------------------------------------------------------------------------------------------------------------------------------
            fprintf(stderr, "Could not connect to a Server: %s\n", gai_strerror(success));
            close(ressources->socketDescriptorWrite);  // connection failed, close socket.
            continue;   // try next pointer
        }
        break; // connection established
    }

    // if the currentPointer is Null at this point, something was going wrong
    if (currentServerAddr == NULL) {
        freeaddrinfo(serveraddr);   // Free the allocated pointer before quitting
        closeAllRessources(ressources);
        errorMessage("Connection failed.", strerror(errno), ressources);
    }
    freeaddrinfo(serveraddr);   // free the allocated pointer


    /* inet_ntop: Convert Internet number in IN to ASCII representation.  The return value
   is a pointer to an internal array containing the string.*/
    fprintf(stdout, "... Connection to Server: established\n");

    /* Here begins the write read loop of the client */

	//---------------------------------------------------------------------------------
	//----------establish reading connection to server (socketDescriptorRead)----------
	//---------------------------------------------------------------------------------
    /* using filepointer instead of write */
    /* convert the file descriptor to a File Pointer */
	//dup returns -1 if failed, descriptor if succeeded
    if ((ressources->socketDescriptorRead = dup(ressources->socketDescriptorWrite)) == -1) {
        closeAllRessources(ressources);
        errorMessage("Error in duplicating file descriptor", strerror(errno), ressources);
    }
    fprintf(stdout, "Copied the socked descr.: orig:%d copy: %d\n", ressources->socketDescriptorWrite,
            ressources->socketDescriptorRead);

	//---------------------------------------------------------------------------------------------------
	//----------open Filepointer to write/read (filepointerClientRead & filepointerClientWrite)----------
	//---------------------------------------------------------------------------------------------------
	//fdopen returns NULL if failed
    ressources->filepointerClientRead = fdopen(ressources->socketDescriptorRead, "r");      // use the copy of the duplicated field descriptor for both file Pointer
    ressources->filepointerClientWrite = fdopen(ressources->socketDescriptorWrite, "w");

    if (ressources->filepointerClientRead == NULL || ressources->filepointerClientWrite == NULL) {
        // an error occured during cerate Filepointer, close the file descriptor
        closeAllRessources(ressources);
        errorMessage("Could not create a File Pointer", strerror(errno), ressources);
    }

    ssize_t sentBytes; //recbytes;
    if (imgUrl == NULL) {
		//fprintf returns bytes written to messageOut
        sentBytes = fprintf(ressources->filepointerClientWrite, "user=%s\n%s", user, messageOut);
    } else {
        sentBytes = fprintf(ressources->filepointerClientWrite, "user=%s\nimg=%s\n%s", user, imgUrl, messageOut);
    }
    // Outcommented we work with the filepointer and fprintf -- then why still here?-----------------------------------------------------------------------------------------------
    //sendbytes = write(fd_socket, messageOut, sizeof(messageOut));
    //if (sendbytes < 0) {
    //    closeAllRessources(ressources);
    //    errorMessage("Could not write to serverIP: ", strerror(errno), ressources);
    //}
    if (sentBytes == -1) {
        // an error occured during write, close the file descriptor
        closeAllRessources(ressources);
        errorMessage("Could not write to the File Pointer", strerror(errno), ressources);
    } else if (sentBytes == 0) {
        closeAllRessources(ressources);
        errorMessage("Nothing sent.", strerror(errno), ressources);
    }
    fflush(ressources->filepointerClientWrite);        //@TODO  ERROR Checking


    /* Close the write connection from the client, nothing to say ... */
    if ((shutdown(fileno(ressources->filepointerClientWrite), SHUT_WR)) < 0) {
        closeAllRessources(ressources);
        errorMessage("Could not close the WR socket: ", strerror(errno), ressources);
    } // outcommented for testing

    fclose(ressources->filepointerClientWrite);
    ressources->filepointerClientWrite = NULL;

    // @TODO: close ressources in error case.
    // Get status
    if (fgets(statusBuffer, STATUSLENGTH, ressources->filepointerClientRead) == NULL) {     // fgets uses read descriptor
        closeAllRessources(ressources);
        errorMessage("Could not read from server: ", strerror(errno), ressources);
    }

    extractedStatus = extractIntfromString(statusBuffer, strlen(statusBuffer));
    fprintf(stdout, "Server says: %s\n", statusBuffer);
    fprintf(stdout, "Status is: %d\n", extractedStatus);

    // get filename
    if (fgets(filenameBuffer, MAXFILENAMELENGTH, ressources->filepointerClientRead) == NULL) {    // fgets uses read descriptor
        closeAllRessources(ressources);
        errorMessage("Could not read from server: ", strerror(errno), ressources);
    }

    if (fgets(htmlLenghtBuffer, MAXFILELENGTH, ressources->filepointerClientRead) == NULL) {
        closeAllRessources(ressources);
        errorMessage("Could not read from server: ", strerror(errno), ressources);
    }
    extractedHtmlFileLength = extractIntfromString(htmlLenghtBuffer, MAXFILELENGTH);

    fprintf(stdout, "Server says: %s\n", filenameBuffer);
    fprintf(stdout, "Server says: %s, %d\n", htmlLenghtBuffer, extractedHtmlFileLength);

    // extract the filename from the field
    char* filename = NULL;
    extractFilename(filenameBuffer, &filename, ressources);
    fprintf(stderr, "Filename: %s", filename);

    // open filepointer for disk
    ressources->clientWriteDiskFp = fopen(filename, "w");

    // we don't need the pointer anymore free it.
    free(filename);
    filename = NULL;

    /* Call the write function */
    writeToDisk(extractedHtmlFileLength, ressources);

    // get filename
    if (fgets(filenameBuffer, MAXFILENAMELENGTH, ressources->filepointerClientRead) == NULL) {    // fgets uses read descriptor
        closeAllRessources(ressources);
        errorMessage("Could not read from serverIP: ", strerror(errno), ressources);
    }
    if (fgets(htmlLenghtBuffer, MAXFILELENGTH, ressources->filepointerClientRead) == NULL) {
        closeAllRessources(ressources);
        errorMessage("Could not read from serverIP: ", strerror(errno), ressources);
    }
    int binary_filelenght = extractIntfromString(htmlLenghtBuffer, MAXFILELENGTH);

    fprintf(stdout, "Server says: %s\n", filenameBuffer);
    fprintf(stdout, "Server says: %s, %d\n", htmlLenghtBuffer, binary_filelenght);

    // extract the filename from the field
    extractFilename(filenameBuffer, &filename, ressources);
    fprintf(stderr, "Filename: %s", filename);
    ressources->clientWriteDiskFp = fopen(filename, "w");

    // we don't need the pointer anymore free it.
    free(filename);
    filename = NULL;

    /* Call the write function */
    writeToDisk(binary_filelenght, ressources);

    /* Close the read connection from the client, over and out ... */
    int lastshutdown;
    if ((lastshutdown = shutdown(fileno(ressources->filepointerClientRead), SHUT_RDWR)) == -1) {
        closeAllRessources(ressources);
        errorMessage("Could not close the RD socket: ", strerror(errno), ressources);
    }
    printf("Last shutdown exit code: %d\n", lastshutdown);

    // CLOSE() - The read end
    if (close(fileno(ressources->filepointerClientRead)) < 0) {
        closeAllRessources(ressources);
        errorMessage("Error in closing socket", strerror(errno), ressources);
    }
    // close the filestream
    fclose(ressources->filepointerClientRead);
    ressources->filepointerClientRead = NULL;

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
            actualRead = fread(restOfFile, 1, rest, ressources->filepointerClientRead);
            if (actualRead == 0) {
                errorMessage("Could not read from server: ", strerror(errno), ressources);
            }
            // check if EOF
            if (feof(ressources->filepointerClientRead) != 0) {
                isEOF = true;
            }
            if (ferror(ressources->filepointerClientRead) != 0) {
                closeAllRessources(ressources);
                errorMessage("Error in reading from socket", strerror(errno), ressources);
            }

            readBytes += actualRead;

            actualWrite = fprintf(ressources->clientWriteDiskFp, "%s", restOfFile);
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

    fflush(ressources->clientWriteDiskFp);
    fclose(ressources->clientWriteDiskFp);  // close the file
    ressources->clientWriteDiskFp = NULL;

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
    // @TODO: return -1 when eror and handle it in caller function------------------------------------------------------------------------------------------------------
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
    if (b >= MAXFILENAMELENGTH - 1) {
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
    if (ressources->socketDescriptorRead != -1) {
        close(ressources->socketDescriptorRead);
        ressources->socketDescriptorRead = -1;
    }
    if (ressources->socketDescriptorWrite != -1) {
        close(ressources->socketDescriptorWrite);
        ressources->socketDescriptorWrite = -1;
    }
    if (ressources->filepointerClientRead != NULL) {
        fclose(ressources->filepointerClientRead);
        ressources->filepointerClientRead = NULL;
    }
    if (ressources->filepointerClientWrite != NULL) {
        fclose(ressources->filepointerClientWrite);
        ressources->filepointerClientWrite = NULL;
    }
    if (ressources->clientWriteDiskFp != NULL) {
        fclose(ressources->clientWriteDiskFp);
        ressources->clientWriteDiskFp = NULL;
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

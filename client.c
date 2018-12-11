/**
 * @file simple_message.client.c
 * @author Valentin Platzgummer - ic17b096
 * @author Lara Kammerer - ic17b001
 * @date 18.11.18
 * @brief Implementation of a simple client application to communicate with a server application
 * TCP/IP Lecture Distrubted Systems
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
#include <netdb.h>          // provides getaddrinfo()
#include <simple_message_client_commandline_handling.h> // provides smc_parsecommandline()
#include <math.h>           // provides floor()
#include <stdbool.h>        // provides true, false
#include <limits.h>         // provide max file length

// --------------------------------------------------------------- defines --
/** @brief length of the field status max 10 */
#define STATUSLENGTH 10
/** @brief length of the field filename max 256 bytes */
#define MAXFILENAMELENGTH _POSIX_PATH_MAX
/** @brief length of the field file length max 10 bytes i.e 10^10 Bytes far enough */
#define MAXFILELENGTH 20
/** @brief chunck size of the reading buffer  */
#define CHUNK 256
/** @brief LINEOUTPUT prints filename, functionname and linenumber from caller */
#define LINEOUTPUT fprintf(stdout, "[%s, %s, %d]: ",  __FILE__, __func__, __LINE__)
/** @brief FIELD_DELIMITER defines the delimiter between the fields defined in the protocol */
#define FIELD_DELIMITER 10
/** @brief FIELD_ASSIGNMENT defines the assignment sign between key and value */
#define FIELD_ASSIGNMENT '='

// -------------------------------------------------------------- typedefs --
/** @brief ressourcesContainer stores all needed ressources in one single place */
typedef struct ressourcesContainer {
    FILE* filepointerClientRead;             /**< File Pointer for Read operation */
    FILE* filepointerClientWrite;            /**< File Pointer for Write operation */
    FILE* filepointerClientWriteDisk;        /**< File Pointer for Hard Disk operation */
    int socketDescriptorRead;                /**< Socket for Read operation */
    int socketDescriptorWrite;               /**< Socket for Write operation */
    const char* progname;                    /**< Program Name argv[0] */
    int verbose;                             /**< Output in verbose mode 0 off, 1 on */
} ressourcesContainer;

// --------------------------------------------------------------- globals --
/** @brief progname char*: stores the program name for correct error codes */
const char* progname;
/** @brief filenamePrefix const char*: the filename prefix used by this protocol */
const char* filenameKey = "file=\0";
/** @brief lengthPrefix const char*: file length prefix used by this protcol */
const char* lengthKey = "len=\0";

// ------------------------------------------------------------- functions --
static void errorMessage(const char* userMessage, const char* errorMessage, ressourcesContainer* ressources);
static void usage(FILE* stream, const char* cmnd, int exitcode);
static int printAddress(struct sockaddr* sockaddr);
static bool writeToDisk(long length, ressourcesContainer* ressources);
static long parseIntfromString(const char* buffer);
static int parseField(char* fieldBuffer, char** filename);
static void closeAllRessources(ressourcesContainer* ressources);

/**
 * @brief main routine of the client implementation sends messages to the server and receive replies.
 * @param argc int: number of program arguments
 * @param argv char**: pointerarray with the given arguments
 * @return int: either 0 in case of success of 1 in case of failure
 */

// --------------------------------------------------------------- main --
int main(int argc, const char* argv[]) {
    struct addrinfo* serveraddr, * currentServerAddr;    // Returnvalue of getaddrinfo(), currentAddr used for loop
    struct addrinfo hints;								 // Hints struct for the addr info function
    char statusBuffer[STATUSLENGTH];					 // Buffer for the Status
    int statusValue;                                 // integer holds status
    char filenameBuffer[MAXFILENAMELENGTH];				 // Buffer for File name max 255 Chars
    char lengthBuffer[MAXFILELENGTH];                // Max size of length
    long fileLengthValue;

    //--------------------------------------------------
    //----------allocate the ressources struct----------
    //--------------------------------------------------
    ressourcesContainer* ressources = malloc(sizeof(ressourcesContainer));
    if (ressources == NULL) {
        fprintf(stderr, "%s: Could not allocate memory: %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    //--------------------------------------------
    //----------set al values to default----------
    //--------------------------------------------
    ressources->filepointerClientRead = NULL;
    ressources->filepointerClientWrite = NULL;
    ressources->filepointerClientWriteDisk = NULL;
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
    int serverPortInt = parseIntfromString(serverPort);

    if ((serverPortInt < 0) || (serverPortInt > 65535)) {
        usage(stderr, "Port outside range", 1);
    }
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Using this options: serverIP: %s, serverPort: %s, messageOut: %s, image_url: %s\n",
                serverIP, serverPort, messageOut, imgUrl);
    }

    // set the progname to the global
    progname = argv[0];

    // Get the type of connection Hint struct!
    memset(&hints, 0, sizeof(hints));   // set to NULL
    hints.ai_family = AF_UNSPEC;            // Allows IP4 and IP6
    hints.ai_socktype = SOCK_STREAM;        // TCP
    hints.ai_protocol = 0;
    hints.ai_flags = 0;
    //----------------------------------------------------
    //----------initialise members of serveraddr----------
    //----------------------------------------------------
    int addrinfoError = 0;
    //getaddrinfo returns 0 if succeeded, works on serveraddr & hints with known serverPort&IP
    if ((addrinfoError = getaddrinfo(serverIP, serverPort, &hints, &serveraddr) !=
            0)) {
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
        if ((printAddress(serveraddr->ai_addr)) == -1) {
            fprintf(stderr, "no address was found");
        }
    }

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
        LINEOUTPUT;
        fprintf(stdout, "Creating a write socket\n");
    }
    //---------------------------------------------------------------------------------------------------
    //----------open Filepointer to write/read (filepointerClientRead & filepointerClientWrite)----------
    //---------------------------------------------------------------------------------------------------
    // fdopen returns NULL if failed
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
    //---------------------------------------------------------------------------------------------------
    //--------------------------- read status field only once -------------------------------------------
    //---------------------------------------------------------------------------------------------------
    char* statusValueRaw;
    int status = parseField(statusBuffer, &statusValueRaw);
    if (status == -1) {
        LINEOUTPUT;
        errorMessage("Error occurred during parsing the status field", strerror(errno), ressources);
    }
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Status buffer: %s Status plain: %s\n", statusBuffer, statusValueRaw);
    }
    statusValue = parseIntfromString(statusValueRaw);
    if (statusValue == -1) {
        LINEOUTPUT;
        errorMessage("Error occurred during converting the file length", strerror(errno), ressources);
    }
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Status is: %d\n", statusValue);
    }
    free(statusValueRaw);

    //---------------------------------------------------------------------------------------------------
    //------------------ begin read field content loop (filenname + file length) ------------------------
    //---------------------------------------------------------------------------------------------------
    bool isEOF = false;
    do {
        int status = 0;
        // get filenameValue from Server
        if (fgets(filenameBuffer, MAXFILENAMELENGTH, ressources->filepointerClientRead) == NULL) {
            if (feof(ressources->filepointerClientRead)) {
                isEOF = true;
                if (ressources->verbose == 1) {
                    LINEOUTPUT;
                    fprintf(stdout, "End of File reached: %d\n", isEOF);
                }
                break;
            } else if (ferror(ressources->filepointerClientRead)) {
                errorMessage("Could not read from server: ", strerror(errno), ressources);
            }
        }
        // get filelength Value from server
        if (fgets(lengthBuffer, MAXFILELENGTH, ressources->filepointerClientRead) == NULL) {
            if (feof(ressources->filepointerClientRead)) {
                isEOF = true;
                if (ressources->verbose == 1) {
                    LINEOUTPUT;
                    fprintf(stdout, "End of File reached: %d\n", isEOF);
                }
                break;
            } else if (ferror(ressources->filepointerClientRead)) {
                errorMessage("Could not read from server: ", strerror(errno), ressources);
            }
        }
        // check if the filenameValue has the correct prefix
        if (strncmp(filenameBuffer, filenameKey, strlen(filenameKey)) != 0) {
            fprintf(stderr, "filenameKey: %s\n", filenameBuffer);
            errorMessage("No filenameKey given.", strerror(errno), ressources);
        }
        char* fileLengthValueRaw = NULL;
        status = parseField(lengthBuffer, &fileLengthValueRaw);
        if (ressources->verbose == 1) {
            LINEOUTPUT;
            fprintf(stdout, "filelength: %s", lengthBuffer);
        }
        if (status == -1) {
            errorMessage("A error occurred during file length parsing\n", strerror(errno), ressources);
        }
        // parse the file length from the incoming buffer
        fileLengthValue = parseIntfromString(fileLengthValueRaw);
        if (fileLengthValue == -1) {
            errorMessage("A error occurred during converting int\n", strerror(errno), ressources);
        }
        if (ressources->verbose == 1) {
            LINEOUTPUT;
            fprintf(stdout, "File length buffer: %s,  length: %ld\n", lengthBuffer, fileLengthValue);
        }
        // extract the filenameKey from the field
        char* filenameValue = NULL;

        // check if the file length has the correct prefix
        if (strncmp(lengthBuffer, lengthKey, strlen(lengthKey)) != 0) {
            errorMessage("No file length given.", strerror(errno), ressources);
        }
        // parse the file length
        status = parseField(filenameBuffer, &filenameValue);
        if (status == -1) {
            errorMessage("A error occurred during filenameKey parsing", strerror(errno), ressources);
        }
        if (ressources->verbose == 1) {
            LINEOUTPUT;
            fprintf(stdout, "Filename: %s\n", filenameValue);
        }
        // open filepointer for disk
        ressources->filepointerClientWriteDisk = fopen(filenameValue, "w");

        // we don't need the pointer anymore free it
        free(fileLengthValueRaw);
        fileLengthValueRaw = NULL;
        free(filenameValue);
        filenameValue = NULL;

        /* Call the write function */
        isEOF = writeToDisk(fileLengthValue, ressources);
    } while (!isEOF);

    //---------------------------------------------------------------------------------------------------
    //------------------ close Filepointer to read (filepointerClientRead) ------------------------------
    //---------------------------------------------------------------------------------------------------

    // close the read filestream and the underlying socket
    if (fclose(ressources->filepointerClientRead) != 0) {
        errorMessage("Could not close the read filestream to socket", strerror(errno), ressources);
    }
    /* Close the read connection from the client, over and out ... */
    ressources->filepointerClientRead = NULL;
    ressources->socketDescriptorRead = -1;
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Closing Filestream Client Read \n");
    }

    // Everything went well, deallocate the ressources struct
    free(ressources);
    return statusValue;
}

/**
* @brief writeToDisk writes received message (information) into known location on disk indicated through filepointerClientWriteDisk
* @param length int: is the length of the received file which is wanted to be written onto the disk
* @param ressources ressourcesContainer*: is a struct containing every information of the used socket, as well as the programname and the information if the output should be verbose
* @return int: 0 if failed, 1 if succeeded
*/
bool writeToDisk(long length, ressourcesContainer* ressources) {
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
    bool isEOF = false;
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Begin the read process.\n");
    }
    //---------------------------------------------------------------------------------------------------
    //------------------ read and write file in block (chunk size) ------- ------------------------------
    //---------------------------------------------------------------------------------------------------

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
        writeBytes += fwrite(partioned_read_array, 1, CHUNK, ressources->filepointerClientWriteDisk);
        if (ferror(ressources->filepointerClientWriteDisk) != 0) {
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
    }

    //---------------------------------------------------------------------------------------------------
    //------------------------- handle the rest if EOF is not set ---------------------------------------
    //---------------------------------------------------------------------------------------------------

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

            actualWrite = fwrite(contentRestOfFile, 1, lengthRest, ressources->filepointerClientWriteDisk);
            writeBytes += actualWrite;
            if (ferror(ressources->filepointerClientWriteDisk) != 0) {
                closeAllRessources(ressources);
                errorMessage("Error in writing to disk", strerror(errno), ressources);
            }
            if (ressources->verbose == 1) {
                LINEOUTPUT;
                fprintf(stdout, "Wrote %d of %d read Bytes to disk\n", actualWrite, actualRead);
            }

            free(contentRestOfFile);   // done with the lengthRest
            contentRestOfFile = NULL;
        }
    }

    //---------------------------------------------------------------------------------------------------
    //------------------ close Filepointer to disk write (filepointerClientWriteDisk) -------------------
    //---------------------------------------------------------------------------------------------------
    fflush(ressources->filepointerClientWriteDisk);
    fclose(ressources->filepointerClientWriteDisk);  // close the file
    ressources->filepointerClientWriteDisk = NULL;

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
    if (ressources->verbose == 1) {
        LINEOUTPUT;
        fprintf(stdout, "Looping the write routine: %d\n", loops);
    }

    return isEOF;
}

/**
* @brief errorMessage prints an error message to the standarderror, then closes all ressources (and frees every pointer) and finally exits the program with an EXIT_FAILURE
* @param userMessage char*: contains the message telling which error has occured
* @param errorMessage char*: contains the errormessage from errno
* @param ressources ressourcesContainer*: is a struct containing every information of the used socket, as well as the programname and the information if the output should be verbose
*/
//no return needed because if function is called, program will be exited in end of the function - return never used
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
 * @brief usage is called upon failure and displays a userinform to stderr and terminates afterwards.
 * @param stream the  stream to write the usage information to (e.g., stdout or stderr).
 * @param cmnd a string containing the name of the executable  (i.e., the contents of argv[0]).
 * @param exitcode the  exit code to be used in the call to exit(3) for terminating the program.
 */
static void usage(FILE* stream, const char* cmnd, int exitcode) {
    fprintf(stream, "usage: %s options\n", cmnd);
    fprintf(stream, "options:\n");
    fprintf(stream, "\t-s, <server>  \tfull qualified domain name or IP address of the server\n");
    fprintf(stream, "\t-p, <port> \twell-known port of the server [0..65535]\n");
    fprintf(stream, "\t-u, <name> \tname of the posting user\n");
    fprintf(stream, "\t-i, <URL> \tURL pointing to an image of the posting user\n");
    fprintf(stream, "\t-m, <message> \tmessage to be added to the bulletin board\n");
    fprintf(stream, "\t-v, \t\tverbose output\n");
    fprintf(stream, "\t-h, \n");

    exit(exitcode);
}

/**
* @brief parseIntfromString parses an integer from the given string wit the prefix 'len='
* @param buffer contains given string with prefix
* @param len contains length of the given string
* @return integer extracted from the given string (buffer) or -1 in case conversion failed
*/
static long parseIntfromString(const char* buffer) {
    long result = 0;
    errno = 0;
    char* eptr = NULL;

    result = strtol(buffer, &eptr, 10);
    if ((eptr == NULL) || (*eptr != '\0')) {
        fprintf(stderr, "%s: Illegal digit\n", progname);
        return -1;
    }
    return result;
}

/**
 * @brief parseField parses a filename from given buffer with the prefix of 'filename='
 * @param fieldBuffer char *, buffer with prefix
 * @param filename char **, pointer to the filename
 * @return 0 in case parsing was successful, -1 if not
 */
static int parseField(char* fieldBuffer, char** filename) {
    int a = 0, b = 0, c = 0;
    // find the asssignment sign
    while (fieldBuffer[a]) {
        if (fieldBuffer[a++] == FIELD_ASSIGNMENT) { break; }
    }
    // find the '\0'b
    b = a;
    while (fieldBuffer[b++]); // find endline '\0', but before is '\n'
    // Filename is too long, print out an error

    char* filenameTemp = NULL;
    // create a new array, b - a is enough because of '\n'
    filenameTemp = malloc((b - a) * sizeof(char));
    if (filenameTemp == NULL) {
        fprintf(stderr, "%s: error allocation memory: %s\n", progname, strerror(errno));
        return -1;
    }
    //copy array -till the delimiter '\n' is found, or till '\0' is found to aviod seg fault
    while ((fieldBuffer[a] != FIELD_DELIMITER) && (fieldBuffer[a])) {
        filenameTemp[c++] = fieldBuffer[a++];
    }
    // Check if Field delimiter is found, if not error occured.
    if (b == a) {
        fprintf(stderr, "%s: no field delimiter is found\n", progname);
        return -1;
    }
    // Termination of the filenameKey
    filenameTemp[c] = '\0';

    *filename = filenameTemp;
    return 0;
}

/**
* @brief closeAllRessources closes all ressources and prints errormessage if an error occurs
* @param ressources is a struct containing every information of the used socket, as well as the programname and the information if the output should be verbose
*/
static void closeAllRessources(ressourcesContainer* ressources) {
    if (ressources->socketDescriptorRead != -1) {
        if (close(ressources->socketDescriptorRead) != 0) {
            fprintf(stderr, "Could not close socketDescriptorRead.\n");
        }
        ressources->socketDescriptorRead = -1;
    }
    if (ressources->socketDescriptorWrite != -1) {
        if (close(ressources->socketDescriptorWrite) != 0) {
            fprintf(stderr, "Could not close socketDescriptorWrite.\n");
        }
        ressources->socketDescriptorWrite = -1;
    }
    if (ressources->filepointerClientRead != NULL) {
        if (fclose(ressources->filepointerClientRead) != 0) {
            fprintf(stderr, "Could not close filepointerClientRead.\n");
        }
        ressources->filepointerClientRead = NULL;
    }
    if (ressources->filepointerClientWrite != NULL) {
        if (fclose(ressources->filepointerClientWrite) != 0) {
            fprintf(stderr, "Could not close filepointerClientWrite.\n");
        }
        ressources->filepointerClientWrite = NULL;
    }
    if (ressources->filepointerClientWriteDisk != NULL) {
        if (fclose(ressources->filepointerClientWriteDisk) != 0) {
            fprintf(stderr, "Could not close clientWriteDiskFp.\n");
        }
        ressources->filepointerClientWriteDisk = NULL;
    }
}

/**
 * @brief printAdress prints the socket address and port to stdout
 * @param sockaddr contains the given socket address
 * @return 0 in case a inet address was found -1 in error case.
 */
static int printAddress(struct sockaddr* sockaddr) {
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
        default:
            return -1;
    }
    return 0;
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

/*
    Brandon Lee
    CS 344 Operating Systems
    Assignment 4 OTP
    otp_dec_d - Daemon background program which performs decoding given plainText and key from port.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// I like bools
typedef int bool;
#define true 1
#define false 0

// List of chracters
const char characterList[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

// Function Delcarations
void initiateServer(int socketFD, struct sockaddr_in serverAddress);
void childHelper(int socketFD, int newSocketFD, char buffer[]);
void socketWrite(int socketFD ,char *data);
int socketReadStream(int socketFD, char *data, long length);
char* decrypt(char* key, char* plainString, int length);
char toChar(int theCharacter);
int toInt(char theCharacter);

int main(int argc, char *argv[]) {

    // Instantiate variables
    char buffer[8192];
    pid_t pid;
    int clientLength;
    int socketFD;
    int newSocketFD;

    // Enable port reuse
    int yes = true;
    setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // Check user arguments
    if (argc != 2) {
        printf("Use Format - otp_dec_d listening_port\n");
        exit(1);
    } else if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) == -1) {

        // Check socket connection
        printf("Server Failure - Cannot open socket!\n");
        exit(1);
    }

    // Instantiate port number and sockets
    int port = atoi(argv[1]);
    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;

    // Set server strcut
    bzero((char *)&serverAddress, sizeof(serverAddress));

    // Boilerplate server structs
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Start it up!
    initiateServer(socketFD, serverAddress);

    // Connection iteration
    while (true) {
        
        // Accept any connection clients
        clientLength = sizeof(clientAddress);
        newSocketFD = accept(socketFD,(struct sockaddr *)&clientAddress, &clientLength);

        // Check for connection issues
        if (newSocketFD < 0) {
            printf("Server Failure - Cannot accept client!\n");
        }

        // Fork!
        pid = fork();

        // Child process
        if (pid == 0) {
            childHelper(socketFD, newSocketFD, buffer);
        } else if (pid < 0) {

            // Fork error
            close(newSocketFD);
            printf("Server Failure: Fork error!\n");

        } else {

            // Parent - End process for new client
            close(newSocketFD);

            // Wait for child
            wait(NULL);
        }
    }

    // Cleanup
    close(socketFD);
    return 0;
}

/*
    Start server connection.
    Parameter: socketFD - Socket file descriptor
    Parameter: serverAddress - Address of server
*/
void initiateServer(int socketFD, struct sockaddr_in serverAddress) {

    // Bind socket to port
    if (bind(socketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1) {
        printf("Server Failure - Port binding!\n");
        exit(1);
    } else if (listen(socketFD, 5) == -1) {

        // Initiate server listening, with limitation at 5
        printf("Server Failure - Connection listening!\n");
        exit(1);
    }
}

/*
    Helper function for child process.
    Parameter: socketFD - Socket file descriptor
    Parameter: newSocketFD - New socket file descriptor
    Parameter: buffer - Buffer string
*/
void childHelper(int socketFD, int newSocketFD, char buffer[]) {

    // Instantiate variables
    int temporaryResult;
    long cipherTextSize;
    long keySize;

    // Close socket and flush buffer
    close(socketFD);
    bzero(buffer, sizeof(buffer));

    // Display to client the server type and re-Flush
    socketWrite(newSocketFD, "dec_d");
    bzero(buffer, sizeof(buffer));

    // Fetch plainText size from client
    temporaryResult = read(newSocketFD, &cipherTextSize, sizeof(cipherTextSize));
    if (temporaryResult < 0) {
        printf("Error - Socket Reading!\n");
        exit(1);
    }

    // Set plainText length and send ACK response
    long cipherTextLength = ntohl(cipherTextSize);
    socketWrite(newSocketFD, "ACK");

    // Allocate memory for plainText, begin plainText socket read stream, and terminate
    char *cipherText = malloc(sizeof(char) * (cipherTextLength + 1));
    long size = socketReadStream(newSocketFD, cipherText, cipherTextLength);
    cipherText[size] = '\0';

    // Send ACK response and flush buffer for client key
    socketWrite(newSocketFD, "ACK");
    bzero(buffer, sizeof(buffer));

    // Fetch client's key size
    temporaryResult = read(newSocketFD, &keySize, sizeof(keySize));
    if (temporaryResult < 0) {
        printf("Error - Socket Reading!\n");
        exit(1);
    }

    // Set key length and send ACK response
    long keyLength = ntohl(keySize);
    socketWrite(newSocketFD, "ACK");

    // Allocate memory for client key and start read stream
    char *key = malloc(sizeof(char) * (keyLength + 1));
    size = socketReadStream(newSocketFD, key, keyLength);
    key[size] = '\0';

    // Send ACK response, decrypt plainText with client key, and send result
    socketWrite(newSocketFD, "ACK");
    char *plainText = decrypt(key, cipherText, strlen(cipherText));
    socketWrite(newSocketFD, cipherText);

    // Cleanup
    close(newSocketFD);
    free(plainText);
    free(key);
    free(cipherText);

    exit(0);
}

/*
    Write data to socket.
    Parameter: socketFD - Socket file descriptor
    Parameter: data - Data to write
*/
void socketWrite(int socketFD, char *data) {

    // Get data size
    long length = (strlen(data) + 1);

    // Send and track result and check for socket writing errors
    int result = write(socketFD, data, length);
    if (result == -1) {
        printf("Server Failure - Socket writing error!\n");
    }
}

/*
    Read data from socket.
    Parameter: socketFD - Socket file descriptor
    Parameter: data - Data to write
    Parameter: length - Data length
    Returns: Bytes read count
*/
int socketReadStream(int socketFD, char *data, long length) {

    // Set buffer for receiving and values to count characters
    char buffer[8192];
    long totalBytes = 0;
    int bytesRead = 0;

    // Set termination
    data[0] = '\0';

    // Iterate
    while (totalBytes < length) {
        bytesRead = read(socketFD, buffer, sizeof(buffer));

        // Error check
        if (bytesRead == 0) {
            printf("Server Failure - Connection closed while socket reading!\n");
        } else if (bytesRead == -1) {
            printf("Server Failure - Socket read error!\n");
        }  else {

            // Set buffer in array and increment
            strncat(data, buffer, bytesRead);
            totalBytes += bytesRead;
        }
    }

    return totalBytes;
}

/*
    Decrypt plainString with key.
    Parameter: key - Encryption key
    Parameter: plainString - The message
    Parameter: length - message length
    Returns: Decrypted String
*/
char* decrypt(char* key, char* encodedString, int length) {

    // Set decryption string
    char* decryptedString = (char*)malloc(length);
    decryptedString[0] = '\0';

    // Establish variables and counters
    int keyCharacter;
    int current;
    int decryptedChar;

    // Increment through string
    int i;
    for (i = 0; i < length; i++) {

        // Break on newlines
        if (encodedString[i] == '\n') {
            break;
        } else {

            // ASCII Character to int conversions
            current = toInt(encodedString[i]);
            keyCharacter = toInt(key[i]);

            decryptedChar = (current - keyCharacter);

            if (decryptedChar < 0) {
                decryptedChar += 27;
            }

            // Decrypt with OTP!
            decryptedChar = (decryptedChar % 27);
            decryptedString[i] = toChar(decryptedChar);
        }
    }

    // Terminate and return
    decryptedString[i] = '\0';
    return decryptedString;
}

/*
    Converts ASCII integer to character.
    Parameter: theCharacter - ASCII integer
    Returns: Converted character
*/
char toChar(int theCharacter) {

    // Iterate for character result
    int i;
    for (i = 0; i < 27; i++) {
        if (theCharacter == i) {
            return characterList[i];
        }
    }
}

/*
    Converts character to ASCII integer.
    Parameter: theCharacter - Character to convert.
    Returns: Converted integer
*/
int toInt(char theCharacter) {

    // Iterate for ASCII result
    int i;
    for (i = 0; i < 27; i++) {
        if (theCharacter == characterList[i]) {
            return i;
        }
    }
}

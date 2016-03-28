/*
    Brandon Lee
    CS 344 Operating Systems
    Assignment 4 OTP
    otp_enc - Connects to respective Daemon program and asks it to start encryption.
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
#include <netdb.h>

// I like bools
typedef int bool;
#define true 1
#define false 0

// Function Delcarations
void writeToSocket(int socketFD, long sendingSize);
void fetchServerResponse(int socketFD, char buffer[]);
void serverOperations(int socketFD, char buffer[], char *plainText, long plainTextSize, char *key, long keySize);
int checkStringCase(char* str);
int socketReadStream(int socketFD, char *data, long len);
void socketWrite(int socketFD, char *data);
void socketRead(int socketFD, char *data);

int main(int argc, char *argv[]) {

    // Instantiate variables
    int socketFD;
    char buffer[8192];

    // Socket structs
    struct sockaddr_in serverAddress;
    struct hostent *server;

    // Check user input
    if (argc != 4) {
        printf("Use Format - otp_enc plainTextFile keyfile port\n");
        exit(1);
    }

    // Initiate file pointer
    FILE *filePointer = fopen(argv[1], "r");

    // Check for file reading problems
    if (filePointer == NULL) {
        printf("Client Failure - Cannot open plaintext file!\n");
        exit(1);
    }

    // Get file size
    fseek(filePointer, 0, SEEK_END);
    long plainTextSize = ftell(filePointer);
    fseek(filePointer, 0, SEEK_SET);

    // Allocate file buffer and read file data into buffer
    char *plainText = (char*)malloc(plainTextSize);
    fread(plainText, plainTextSize, 1, filePointer);

    // Add terminator to EOF
    plainText[plainTextSize - 1] = '\0';

    // Close file
    fclose(filePointer);

    // Check if plaintext characters are valid
    if (checkStringCase(plainText) == 1) {
        printf("File Failure - The following file contains bad characters: \"%s\"\n", argv[1]);
        exit(1);
    }

    // Open key file
    filePointer = fopen(argv[2], "r");

    // Get file size
    fseek(filePointer, 0, SEEK_END);
    long keySize = ftell(filePointer);
    fseek(filePointer, 0, SEEK_SET);

    // Allocate file buffer and read file data into buffer
    char *key = (char*)malloc(keySize);
    fread(key, keySize, 1, filePointer);

    // Add terminator to EOF
    key[keySize - 1] = '\0';

    // Close file
    fclose(filePointer);

    // Check if key is valid with plainText
    if (keySize < plainTextSize) {
        printf("File Failure - The following key is too short: \"%s\"\n", argv[2]);
        exit(1);
    }

    // Check the socket connection
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Client Failure - Socket opening failure!\n");
        exit(1);
    }

    // Check if file has uppercase or invalid characters
    if (checkStringCase(key) == 1) {
        printf("File Failure - The following file contains bad characters: \"%s\"\n", argv[2]);
        exit(1);
    }

    // Get server hostname and check for errors
    server = gethostbyname("localhost");
    if (server == NULL) {
        printf("Client Failure: Invalid host!\n");
        exit(2);
    }

    // Set port number
    int port = atoi(argv[3]);

    // Server housekeeping - Zero out and copy
    bzero((char *)&serverAddress, sizeof(serverAddress));
    bcopy((char *)server->h_addr, (char *)&serverAddress.sin_addr.s_addr, server->h_length);

    // Struct Boilerplate
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    // Try to connect and catch error if failure
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        printf("Client Failure - Error connecting to server!\n");
        exit(2);
    }

    // Fetch server type
    fetchServerResponse(socketFD, buffer);

    // Double check connection
    if (strcmp(buffer, "enc_d") != 0) {
        printf("Client Failure - Cannot create connection to port %d\n", port);
        exit(2);
    }

    // Initiate plainText requests
    serverOperations(socketFD, buffer, plainText, plainTextSize, key, keySize);

    // Create space for encrypted text, get/return response
    char *cipherText = malloc(sizeof(char) * (plainTextSize + 1));
    long cipherTextSize = socketReadStream(socketFD, cipherText, plainTextSize);
    cipherText[cipherTextSize] = '\0';

    // Print
    printf("%s\n", cipherText);

    // Cleanup and return
    close(socketFD);
    free(plainText);
    free(key);
    free(cipherText);
    return 0;
}

/*
    Helper function to write to the socket and check if the result is valid.
    Parameter: socketFD - Socket file descriptor
    Parameter: sendingSize - Writing size
*/
void writeToSocket(int socketFD, long sendingSize) {
    int temporaryResult = write(socketFD, &sendingSize, sizeof(sendingSize));
    if (temporaryResult < 0) {
        printf("Client Failure - Error writing to socket!\n");
    }
}

/*
    Helper function to get response from the server.
    Parameter: socketFD - Socket file descriptor
    Parameter: buffer - The string buffer
*/
void fetchServerResponse(int socketFD, char buffer[]) {

    // Zero out buffer
    bzero(buffer, sizeof(buffer));
    socketRead(socketFD, buffer);
}

/*
    Helper function to send requests to the server.
    Parameter: socketFD - Socket file descriptor
    Parameter: buffer - The string buffer
    Parameter: plainText - The message
    Parameter: plainTextSize - The length of the plainText
    Parameter: key - The key
    Parameter: keySize - The length of key
*/
void serverOperations(int socketFD, char buffer[], char *plainText, long plainTextSize, char *key, long keySize) {

    // Get the sending size
    long sendingSize = htonl(plainTextSize);

    // Send to socket, request response from server, send key to server, and finally request response from server
    writeToSocket(socketFD, sendingSize);
    fetchServerResponse(socketFD, buffer);
    socketWrite(socketFD, plainText);
    fetchServerResponse(socketFD, buffer);

    // Begin key operations - start sending to server
    sendingSize = htonl(keySize);

    // Send to socket, request response from server, send key to server, and finally request response from server
    writeToSocket(socketFD, sendingSize);
    fetchServerResponse(socketFD, buffer);
    socketWrite(socketFD, key);
    fetchServerResponse(socketFD, buffer);
}

/*
    Checks string for invalid characters.
    Parameter: string - The string
    Returns: Boolean
*/
int checkStringCase(char* string) {
    int stringCase = 0;
    int i = 0;
    while(string[i] != '\0') {
        if ((string[i] >= 'a' && string[i] <= 'z')) {
            stringCase = 1;
        }
        i++;
    }
    return stringCase;
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
            exit(1);
        } else if (bytesRead == -1) {
            printf("Server Failure - Socket read error!\n");
            exit(1);
        }  else {

            // Set buffer in array and increment
            strncat(data, buffer, bytesRead);
            totalBytes += bytesRead;
        }
    }

    return totalBytes;
}

/*
    Write data to socket.
    Parameter: socketFD - Socket file descriptor
    Parameter: data - Data to write
*/
void socketWrite(int socketFD, char *data) {

    // Get data size
    long length = (strlen(data) + 1);

    // Send/track result and check for errors
    int result = write(socketFD, data, length);
    if (result == -1) {
        printf("Server Failure - Socket writing error!\n");
    }
}

/*
    Fetch data from connection.
    Parameter: socketFD - Socket file descriptor
    Parameter: data - The data
*/
void socketRead(int socketFD, char *data) {
    long length = 8192;

    // Read and store
    int result = read(socketFD, data, length);

    // Check for errors
    if (result == 0) {
        printf("Socket Failure - Connection closed while reading!\n");
        exit(1);
    } else if (result == -1) {
        printf("Socket Failure - Error reading from socket!\n");
        exit(1);
    }
}

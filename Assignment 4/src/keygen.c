/*
    Brandon Lee
    CS 344 Operating Systems
    Assignment 4 OTP
    keygen - Generates a random key
*/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

void generateKey(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    generateKey(argc, argv);
    return 0;
}

/*
    Creates a random key given a specified length.
    Parameter: argv[1] - The key length
*/
void generateKey(int argc, char *argv[]) {
    char characterList[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
    time_t theTime;

    // Set random seed
    srand((unsigned)time(&theTime));

    //Get our key length
    int keylength = atoi(argv[1]);

    // Check for missing parameters
    if (argc != 2) {
        printf("Command Format: keygen keylength\n");
        exit(1);
    }

    // Create string
    char* theKey = (char*)malloc(sizeof(char)*(keylength + 1));

    // Iterate
    int i;
    for (i = 0; i < keylength; i++) {
        theKey[i] = characterList[rand() % 27];
    }

    // Terminate
    theKey[keylength] = '\0';

    printf("%s\n", theKey);

    // Cleanup
    free(theKey);
}

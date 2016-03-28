/**
    Brandon Lee
    Assignment 3 - smallsh
    CS 344 - Operating Systems
*/

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

// Because I like bools :D
typedef int bool;
#define true 1
#define false 0

// Input commmand representation
typedef struct command {

    // User inputed strings - files, commands, and arguments
    char *inFile;
    char *outFile;
    char *userCommand;
    char *userArgs;

    // Redirection and background markers
    bool redirectIn;
    bool redirectOut;
    bool backgroundProcess;

} command;

// Background process - single linked list
typedef struct backPID {
    int childPID;

    // Next link
    struct backPID *next;
} backPID;


// Setup single linked list
backPID *head, *current;

// Setup internal commands
char *internalCommands[] = {"cd", "status", "exit"};

// Set shell to globally be running
bool ACTIVESHELL = true;

// Exit status flag for most recent fg command struct
bool EXITSTATUS = false;

// Signal flag for most recent signal
bool SIGNALSTATUS = false;

// Function declarations
void initializeShell();
command *getInput(int argBuffer, int lineBuffer);
void specialCharInputHelper(command *theCommand, int argBuffer, char *argumentBuffer, char *inputRedirect, char *outRedirect);
void processManager(command *theCommand);
void checkBackground();
bool isInternalCommand(char *theCommand);
void executeInternalCommand(command *theCommand);
void execute(command *theCommand);
void executionRedirectionHelper(command *theCommand, int fd);
void executeHelper(command *theCommand, int tokenBuffer, int index);
void childManagementHelper(command *theCommand, pid_t pid, pid_t wpid, int status);
void cleanShell();

int main() {

    // Set up shell
    initializeShell();

    // Run shell until ACTIVESHELL ends
    while (ACTIVESHELL) {
        // Prompt user and flush
        printf(" : ");
        fflush(stdout);

        //Get command from user and pass it to our process manager
        processManager(getInput(512, 2048));
    }

    // Clean up
    cleanShell();

    return EXIT_SUCCESS;
}

/**
    Start up shell, set up link listss and sigaction for process handling.
*/
void initializeShell() {

    // Setup linked list
    head = (backPID *)malloc(sizeof(backPID));
    current = head;
    current->childPID = -1;
    current->next = NULL;

    // Enable signals
    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = SIG_IGN;
    sigaction(SIGINT, &act, NULL);
}

/**
    Reads user input from buffer.

    Parameter argumentBuffer: The maximum argument buffer
    Parameter lineBuffer: The maximum command buffer
    Returns: Command struct
*/
command *getInput(int argBuffer, int lineBuffer) {

    // Set the Newline flag
    bool newLine = false;

    // Set buffers for input
    char *commandBuffer = (char*) malloc(sizeof(char) * lineBuffer);
    char *argumentBuffer = (char*) malloc(sizeof(char) * argBuffer);
    char *inputRedirect = (char*) malloc(sizeof(char) * argBuffer);
    char *outRedirect = (char*) malloc(sizeof(char) * argBuffer);

    //Initalize command structure - set redirection to false and files as NULL
    command *theCommand = (command *)malloc(sizeof(command));
    theCommand->userCommand = (char*) malloc(sizeof(char) * lineBuffer);
    theCommand->userArgs = (char*) malloc(sizeof(char) * argBuffer);
    theCommand->backgroundProcess = false;
    theCommand->redirectIn = false;
    theCommand->redirectOut = false;
    theCommand->inFile = NULL;
    theCommand->outFile = NULL;

    // Ignore whitespace
    while (true) {
        int currentChar = getchar();

        // Check for space
        if (!isspace(currentChar)) {
            // Put character back
            ungetc(currentChar, stdin);
            break;
        }

        // Check EOF or newline
        if (currentChar == '\n' || currentChar == EOF) {
            commandBuffer[0] = '\0';
            newLine = 1;
            break;
        }
    }

    // The input command
    int i = 0;
    while (!newLine) {
        int currentChar = getchar();

        // Check character for if a space, newline, EOF, or full buffer
        if (isspace(currentChar) || currentChar == '\n' || currentChar == EOF || (i == lineBuffer - true)) {
            if (currentChar == EOF || currentChar == '\n') {

                // Empty the command arguments and set newLine to true
                theCommand->userArgs[0] = '\0';
                newLine = true;
            }

            // End the string and break
            commandBuffer[i] = '\0';
            break;
        }

        // Increment and move on
        commandBuffer[i] = currentChar;
        i++;

        // Check the buffer size and increase if needed
        if (lineBuffer <= i) {
            lineBuffer += lineBuffer;

            // Update the command and command buffer
            theCommand->userCommand = realloc(theCommand->userCommand, (lineBuffer * sizeof(char)));
            commandBuffer = realloc(commandBuffer, (lineBuffer * sizeof(char)));
        }
    }

    // The input arguments
    i = 0;

    // Set flag for special characters
    bool special = false;

    while (!newLine) {
        int currentChar = getchar();

        // Check character for if newline, EOF, or full
        if (currentChar == '\n' || currentChar == EOF || (i == lineBuffer - 1)) {

            // End the string and break
            argumentBuffer[i] = '\0';
            break;
        }

        // Check for special symbol
        if (currentChar == '&'|| currentChar == '<' || currentChar == '>') {
            special = true;
        }

        // Increment and move on
        argumentBuffer[i] = currentChar;
        i++;

        // Check the buffer size and increase if needed
        if (argBuffer <= i) {
            argBuffer += argBuffer;

            //Update the redirectors and argumentBuffer
            inputRedirect = realloc(inputRedirect, (argBuffer * sizeof(char)));
            outRedirect = realloc(inputRedirect, (argBuffer * sizeof(char)));
            argumentBuffer = realloc(argumentBuffer, (argBuffer * sizeof(char)));
        }
    }

    // Check for special characters
    if (special) {
        specialCharInputHelper(theCommand, argBuffer, argumentBuffer, inputRedirect, outRedirect);
    }

    // Transfer command and argument buffers to command struct
    strcpy(theCommand->userCommand, commandBuffer);

    // Collect redirection data
    if (theCommand->redirectOut) {
        theCommand->outFile = (char*)malloc(sizeof(outRedirect) / sizeof(char*));
        strcpy(theCommand->outFile, outRedirect);
    }

    if (theCommand->redirectIn) {
        theCommand->inFile = (char*)malloc(sizeof(inputRedirect) / sizeof(char*));
        strcpy(theCommand->inFile, inputRedirect);
    }

    if (!newLine) {
        strcpy(theCommand->userArgs, argumentBuffer);
    }

    // Cleanup and terminate
    free(inputRedirect);
    free(outRedirect);
    free(commandBuffer);
    free(argumentBuffer);
    fflush(stdout);
    return theCommand;
}


/**
    Helper function for getInput() special character inputs.

    Parameter theCommand: Command struct
    Parameter argBuffer: Buffer size
    Parameter argumentBuffer: Actual argumentBuffer string
    Parameter inputRedirect: input redirection string
    Parameter outRedirect: output redirection string
*/
void specialCharInputHelper(command *theCommand, int argBuffer, char *argumentBuffer, char *inputRedirect, char *outRedirect) {

    // Starting location, background process, Input redirection, output redirection
    int startChar = -1;
    int backChar = -1;
    int inChar = -1;
    int outChar = -1;

    // Find the location of each special character
    int i = 0;
    for (i = 0; i < strlen(argumentBuffer); i++) {
        if (argumentBuffer[i] == '&') {

            // Check if this is the special character start location
            if (startChar == -1 && backChar == -1 && inChar == -1 && outChar == -1) {
                startChar = i;
            }

            // Update command struct with updated location
            backChar = i;
            theCommand->backgroundProcess = true;
        }

        if (argumentBuffer[i] == '<') {

            // Check if this is the special character start location
            if (startChar == -1 && backChar == -1 && inChar == -1 && outChar == -1) {
                startChar = i;
            }

            // Update command struct with updated location
            inChar = i;
            theCommand->redirectIn = true;
        }

        if (argumentBuffer[i] == '>') {

            // Check if this is the special character start location
            if (startChar == -1 && backChar == -1 && inChar == -1 && outChar == -1) {
                startChar = i;
            }

            // Update command struct with updated location
            outChar = i;
            theCommand->redirectOut = true;
        }
    }

    // Get data from output redirection
    if (-1 < outChar) {
        int k = 0;
        for (i = outChar + 1; i < strlen(argumentBuffer); i++) {

            // Permit space after redirection
            if (!isspace(argumentBuffer[i])) {
                outRedirect[k] = argumentBuffer[i];
                k++;
            }

            // Break on space (after the first character)
            if (isspace(argumentBuffer[i]) && 0 < k) {
                break;
            }
        }

        // End the string
        outRedirect[k] = '\0';
    }

    // Get data from input redirection
    if (-1 < inChar) {
        int k = 0;
        for (i = inChar + 1; i < strlen(argumentBuffer); i++) {

            // Permit space after redirection
            if (!isspace(argumentBuffer[i])) {
                inputRedirect[k] = argumentBuffer[i];
                k++;
            }

            //Break on space after first char
            if (isspace(argumentBuffer[i]) && 0 < k){
                break;
            }
        }

        // End the string
        inputRedirect[k] = '\0';
    }

    // Clear argument buffer's special characters
    char *newBuffer = (char*) malloc(sizeof(char) * argBuffer);
    strncpy(newBuffer, argumentBuffer, startChar);
    newBuffer[startChar] = '\0';

    // Update and cleanup
    free(argumentBuffer);

    argumentBuffer = (char*) malloc(sizeof(char) * argBuffer);
    strcpy(argumentBuffer, newBuffer);

    free(newBuffer);
}

/**
    Interpret and process commands

    Parameter theCommand: Command struct
*/
void processManager(command *theCommand) {

    //Check background status
    checkBackground();

    // Don't look at blank commands and comments
    if (theCommand->userCommand[0] != '\0' && theCommand->userCommand[0] != '#') {

        // Determine if command is internal or not
        if (isInternalCommand(theCommand->userCommand)) {

            // Execute internal command
            executeInternalCommand(theCommand);
        } else {

            // Execute non-built in command
            execute(theCommand);
        }
    }
}

/**
    Review status of child (background) processes.
*/
void checkBackground() {
    int pid;
    int status;

    // Check for finished processes
    do {
        pid = waitpid(-1, &status, WNOHANG);

        // Has a background process has died?
        if (0 < pid) {
            printf("Background PID %d is done: ", pid);

            // Check and print if its a signal exit or a normal exit
            if (WIFSIGNALED(status) != 0) {
                printf("terminated by signal %d\n", WTERMSIG(status));
            }
            if (WIFEXITED(status)) {
                printf("exit value %d\n", WEXITSTATUS(status));
            }
        }
    } while (0 < pid);
}

/**
    Checks if a given command is internally handled by the shell.

    Parameter theCommand: Command string to check
    Returns: Boolean
*/
bool isInternalCommand(char *theCommand) {
    int i;
    for (i = 0; i < 3; i++) {
        if (!strcmp(internalCommands[i], theCommand)) {
            return true;
        }
    }

    return false;
}

/**
    Execute interally defined shell command.

    Parameter theCommand: Command struct
*/
void executeInternalCommand(command *theCommand) {

    // Check for the appropriate command to execute
    if (!strcmp(theCommand->userCommand, "status")) {

        // Show either the signal status or exit status
        if (SIGNALSTATUS == false) {
            printf("Exit value %d\n", EXITSTATUS);
        } else {
            printf("Terminated by signal %d\n", SIGNALSTATUS);
        }
    } else if (!strcmp(theCommand->userCommand, "cd")) {

        // Set to the home path if not args are presented.
        if (theCommand->userArgs[0] == '\0') {

            // Get home value
            char *home = getenv("HOME");

            // Check if it exists
            if (home != NULL) {

                // Change the directory
                chdir(home);
            }
        // Otherwise, change to given path if there.
        } else if (chdir(theCommand->userArgs) != false) {

            // Return the missing path
            printf("Cannot find \"%s\"\n", theCommand->userArgs);
            EXITSTATUS = true;
        }
    } else if (!strcmp(theCommand->userCommand, "exit")) {

        // End the shell
        ACTIVESHELL = false;
    }
}

/**
    The main function for executing external shell command.

    Parameter theCommand: Command struct
*/
void execute(command *theCommand) {
    int status;

    // Fork PID and wait PID
    pid_t pid;
    pid_t wpid;

    // Forking magic
    pid = fork();

    // Index and buffer variables for later 2D array style execution
    int index = 0;
    int tokenBuffer = 64;

    // Child process
    if (pid == 0) {

        // Enable signals since child processes utilizes signals
        struct sigaction act;
        act.sa_flags = 0;
        act.sa_handler = SIG_DFL;
        sigaction(SIGINT, &act, 0);

        // File Descriptor
        int fd;

        // Is there any redirection in this command?
        if (theCommand->redirectIn || theCommand->redirectOut) {

            // Execute redirection
            executionRedirectionHelper(theCommand, fd);
        } else if (!theCommand->redirectIn && theCommand->backgroundProcess) {

            // Check for background process with undefined redirection input - Redirect to /dev/null
            fd = open("/dev/null", O_RDONLY, 0644);

            // Check for and print errors
            if (fd == -1) {
                printf("Can't open /dev/null for stdin redirection!\n");
                exit(1);
            } else {

                // Redirect stdin, otherwise print error and exit
                if (dup2(fd, 0) == -1) {
                    printf("Can't redirect stdin to /dev/null!\n");
                    exit(1);
                }

                close(fd);
            }
        }

        executeHelper(theCommand, tokenBuffer, index);

    } else if (0 > pid) {

        // Print fork error
        printf("Fork error!\n");
        EXITSTATUS = true;
    } else {

        // Take care of your children!
        childManagementHelper(theCommand, pid, wpid, status);
    }
}

/**
    Helper function for execute(), establishes redirection for command.

    Parameter theCommand: Command struct
    Parameter fd: file descriptor
*/
void executionRedirectionHelper(command *theCommand, int fd) {
    // Check for output redirection
    if (theCommand->redirectOut) {

        // Create output file
        fd = open(theCommand->outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        // Check for and print errors
        if (fd == -1) {
            printf("The following file cannot be opened for stdout redirection: %s\n", theCommand->outFile);
            exit(1);
        } else {

            // Redirect stdin, otherwise print error and exit
            if (dup2(fd, 1) == -1) {
                printf("Cannot redirect stdout!\n");
                exit(1);
            }

            close(fd);
        }
    }

    // Check for input redirection
    if (theCommand->redirectIn) {

        // Create input file
        fd = open(theCommand->inFile, O_RDONLY, 0644);

        // Check for and print errors
        if (fd == -1) {
            printf("The following file cannot be opened for stdin redirection: %s\n", theCommand->inFile);
            exit(1);
        } else {

            // Redirect stdin, otherwise print error and exit
            if (dup2(fd, 0) == -1) {
                printf("Could not redirect stdin!\n");
                exit(1);
            }

            close(fd);
        }
    }
}

/**
    Helper function for execute(), establishes format for execution with 2D array
    and attempts to execute user's command.
    Array format: 0 - command, 1 to n-1 are the arguments, and n is NULL.

    Parameter theCommand: Command struct
    Parameter tokenBuffer: The buffer size
    Parameter index: Index counter
*/
void executeHelper(command *theCommand, int tokenBuffer, int index) {

    // Allocate 2D array for executing command arguments
    char **userArgs = malloc(sizeof(char*) * tokenBuffer);

    // Set userArgs[0] to the command
    userArgs[index] = theCommand->userCommand;
    index++;

    // Put together with strtok
    char *token = strtok(theCommand->userArgs, " ");
    while (token != NULL) {

        // Go through and increment
        userArgs[index] = token;
        index++;

        // Check token buffer and more memory if needed
        if (tokenBuffer <= index) {
            tokenBuffer += tokenBuffer;
            userArgs = realloc(userArgs, (tokenBuffer * sizeof(char*)));
        }

        // Continue setting tokens
        token = strtok(NULL, " ");
    }

    // Set last index to null for execution
    userArgs[index] = NULL;

    // Assuming everything is ok, execute the 2D array of command and arguments
    if (execvp(userArgs[0], userArgs) == -1) {

        // If not, print error and quit
        printf("Command not recognized!\n");
        exit(1);
    }
}

/**
    Working with parent - need to manage and wait for children.

    Parameter theCommand: Command struct
    Parameter pid: Process ID
    Parameter wpid: Waiting Process ID
    Parameter status: Status keeper
*/
void childManagementHelper(command *theCommand, pid_t pid, pid_t wpid, int status) {

    // Main child management loop
    do {
        // Check if background process
        if (theCommand->backgroundProcess) {

            // If so, add the background process
            while (head->next != NULL) {
                head = head->next;
            }

            // Allocate new node
            head->next = (backPID *)malloc(sizeof(backPID));

            // Set PID in current node
            head->childPID = pid;
            head = head->next;

            // End list
            head->next = NULL;

            // Print background process start message
            printf("Background PID is %d\n", pid);
            fflush(stdout);
        } else {
            // Wait for processes in the foreground
            wpid = waitpid(pid, &status, WUNTRACED);
        }

    } while (!WIFSIGNALED(status) && !WIFEXITED(status));

    // Set forground procedure status and exit values
    if (WIFEXITED(status)) {
        EXITSTATUS = WEXITSTATUS(status);
        SIGNALSTATUS = false;
    } else if (WIFSIGNALED(status)) {
        EXITSTATUS = true;
        SIGNALSTATUS = WTERMSIG(status);
    }
}

/**
    Cleans up all the processes (foreground and background)
*/
void cleanShell() {

    // Kill running processes
    current = head;
    while (current != NULL) {

        // Skip head
        if (current->childPID != -1) {
            kill(current->childPID, SIGKILL);
        }

        // Increment
        current = current->next;
    }

    // Clean up linked lists
    while ((current = head) != NULL) {
        head = head->next;
        free(current);
    }
}

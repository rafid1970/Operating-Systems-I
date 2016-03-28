// Brandon Lee
// Assignment 2 - adventure

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <math.h>

// The room names - all convenient to type xD
char* roomNames[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};

// The room types
typedef enum {START_ROOM, MID_ROOM, END_ROOM} roomType;

// I'm a fan of booleans
typedef int bool;
#define true 1
#define false 0

// Room data type
// roomName           - Name of room
// connections        - Array of connections
// definedConnections - Total of defined connections
// totalConnections   - Total number of all connections
// roomType           - Type of room
typedef struct {
    char *roomName;
    int *connections;
    int definedConnections;
    int totalConnections;
    roomType type;
} Room;

// State container data type
// roomName         - Name of room
// connections      - Array of connections
// totalConnections - Total number of connections
// roomType         - Type of room
typedef struct {
    char *roomName;
    char **connections;
    int totalConnections;
    roomType type;
} GameState;

// Function Declarations
void initiateGame();
void createFiles();
Room *createRooms(int numberOfRooms);
int *createRandomArray(int randomMax, int size);
bool createConnections(int numberOfRooms, Room *roomList);
void createRoomConnections(int numberOfRooms, Room *roomList, int k, int currentIndex);
GameState *initiateFiles();
void fetchContents(FILE *theFile, GameState *theGame, int currentRoom);
void checkMatch(FILE *theFile, GameState *theGame, int currentRoom, char *keyIndex, char *valueIndex);
void gameLoop(int currentRoom, int endRoom, GameState *roomList);
bool checkConnection(char* roomName, GameState* array, int room1);
bool isValueInArray(int value, int *array, int size);
void endMessage(char **userPath, int totalSteps);


int main() {
    initiateGame();
    return 0;
}

/*
Function   - initiateGame(): Sets up GameState and a few initiate functions
Parameters - N/A
Returns    - N/A
*/
void initiateGame() {
    // Initiate gamestate
    GameState *roomList;

    // Instantiate random
    srand(time(NULL) + rand());

    // Create the files and build the game
    createFiles();
    roomList = initiateFiles();

    // Initiate rooms
    int currentRoom;
    int endRoom;

    // Set special rooms
    int i;
    for (i = 0; i < 7; i++) {
        if (roomList[i].type == START_ROOM) {
            currentRoom = i;
        }
        if (roomList[i].type == END_ROOM) {
            endRoom = i;
        }
    }

    // Loop the game
    gameLoop(currentRoom, endRoom, roomList);
}

/*
Function   - createFiles(): Creates directories/files and fills them with data
Parameters - N/A
Returns    - N/A
*/
void createFiles() {

    // Instantiate 7 rooms
    Room *roomList = createRooms(7);

    // Get process id and create directory
    int pid = getpid();
    char directory[60];
    sprintf(directory, "leebran.rooms.%d/", pid);
    mkdir(directory, 0777);

    // For each instantiated room, write the room details!
    int i;
    for (i = 0; i < 7; i++) {

        // Open file and append roomName
        FILE *theFile = fopen((strcat(directory, roomList[i].roomName)), "w+");
        fprintf(theFile, "ROOM NAME: %s\n", roomList[i].roomName);

        // For each connection spot, append the connection
        int j;
        for (j = 0; j < roomList[i].totalConnections; j++) {
            fprintf(theFile, "CONNECTION %d: %s\n", j, roomList[roomList[i].connections[j]].roomName);
        }

        // Stringify room type
        char *roomTypeStr = "";
        if (roomList[i].type == MID_ROOM) {
            roomTypeStr = "MID_ROOM";
        }
        if (roomList[i].type == START_ROOM) {
            roomTypeStr = "START_ROOM";
        }
        if (roomList[i].type == END_ROOM) {
            roomTypeStr = "END_ROOM";
        }

        // Append it and close
        fprintf(theFile, "ROOM TYPE: %s\n", roomTypeStr);
        fclose(theFile);
    }
}

/*
Function   - createRooms(): Create an array of rooms
Parameters - numberOfRooms
Returns    - array
*/
Room *createRooms(int numberOfRooms) {

    // Instantiate our room array
    Room *roomList = (Room *)malloc(sizeof(Room) * numberOfRooms);

    // Randomly create room name array
    int *randomRooms = createRandomArray(10, numberOfRooms);

    // Set random values in!
    int i;
    for (i = 0; i < numberOfRooms; i++) {
        roomList[i].roomName = roomNames[randomRooms[i]];
    }

    // Create types for each room!
    // Set each room with MID_ROOM status
    int j;
    for (j = 0; j < numberOfRooms; j++) {
        roomList[j].type = MID_ROOM;
    }

    // Still need to add start and end rooms..
    // Randomly choose two rooms to be the special rooms
    int *specialRooms = createRandomArray(numberOfRooms, 2);
    roomList[specialRooms[0]].type = START_ROOM;
    roomList[specialRooms[1]].type = END_ROOM;

    // While more connections are needed
    bool moreConnections = false;
    while (!moreConnections) {

        // Make more!
        moreConnections = createConnections(numberOfRooms, roomList);
    }

    return roomList;
}

/*
Function   - createRandomArray(): Creates random array with a given max value
Parameters - randomMax, size
Returns    - array
*/
int *createRandomArray(int randomMax, int size) {

    // Create array
    int *array = (int *)malloc(sizeof(int) * size);

    // Fill with null values
    int i;
    for (i = 0; i < size; i++) {
        array[i] = -1;
    }

    int j;
    for (j = 0; j < size; j++) {

        // Generate random values
        int value = rand() % randomMax;

        // If its in array
        if (isValueInArray(value, array, size) == true) {

            // Nope out
            j--;
            continue;
        }
        // Insert the random value if its not a duplicate
        array[j] = value;
    }

    return array;
}

/*
Function   - createConnections(): Main function to create connections in the game.
Parameters - numberOfRooms, roomList
Returns    - bool
*/
bool createConnections(int numberOfRooms, Room *roomList) {

    // Create for each room
    int i;
    for (i = 0; i < numberOfRooms; i++) {

        // More throwing random numbers until it works xD
        int theConnection;
        while (true) {
            theConnection = rand() % 7;
            if (theConnection > 3) {
                break;
            }
        }

        // Initiate room connections
        roomList[i].connections = (int *)malloc(sizeof(int) * theConnection);

        // Fill connection list with null values
        int j;
        for (j = 0; j < theConnection; j++) {
            roomList[i].connections[j] = -1;
        }

        // Room attribute setup
        roomList[i].totalConnections = theConnection;
        roomList[i].definedConnections = 0;
    }

    // Create connections for each individual room
    int k;
    for (k = 0; k < numberOfRooms; k++) {

        // Get current room index and make connections!
        int currentIndex = roomList[k].definedConnections;
        createRoomConnections(numberOfRooms, roomList, k, currentIndex);
    }

    return true;
}

/*
Function   - createRoomConnections(): Helper function, creates individual room connections.
Parameters - numberOfRooms, roomList, int, int
Returns    - N/A
*/
void createRoomConnections(int numberOfRooms, Room *roomList, int k, int currentIndex) {

    // For any possible connection spots
    for (currentIndex; currentIndex < roomList[k].totalConnections; currentIndex++) {

        // Continuously throw random numbers until it works
        while (true) {
            int randomConnection = rand() % numberOfRooms;

            // Check if random number works
            if (!isValueInArray(randomConnection, roomList[k].connections, roomList[k].totalConnections) && randomConnection != k) {

                // If it works, create the connection, a connection back, increment both rooms, and finish
                roomList[k].connections[currentIndex] = randomConnection;
                roomList[randomConnection].connections[roomList[randomConnection].definedConnections] = k;
                roomList[k].definedConnections++;
                roomList[randomConnection].definedConnections++;
                break;
            }
        }
    }
}

/*
Function   - initiateFiles(): Main function for file io
Parameters - N/A
Returns    - N/A
*/
GameState *initiateFiles() {

    // Create root directory path and pid
    char rootPath[30];
    int pid = getpid();
    sprintf(rootPath, "leebran.rooms.%d/", pid);

    // Instantiate directory and open it up
    DIR *directory;
    struct dirent *directoryEntry;
    directory = opendir(rootPath);

    // Skip the first two, "." and ".." :D
    readdir(directory);
    readdir(directory);

    // Instantiate GameState and currentRoom counter
    GameState *theGame = (GameState *)malloc(sizeof(GameState) * 7);
    int currentRoom = 0;

    // Read all the things in our directory
    while ((directoryEntry = readdir(directory)) != NULL) {

        // Get the full file path
        char fullPath[60];
        strcpy(fullPath, rootPath);
        strcat(fullPath, directoryEntry->d_name);

        // Set up room
        theGame[currentRoom].connections = (char **)malloc(sizeof(char *) * 6);
        theGame[currentRoom].totalConnections = 0;

        // Open file sir!
        FILE *theFile = fopen(fullPath, "r");

        // Pull all the data out and increment!
        fetchContents(theFile, theGame, currentRoom);
        currentRoom++;
    }

    // Closeup and finish.
    closedir(directory);
    return theGame;
}

/*
Function   - fetchContents(): Helper function
Parameters - FILE, GameState, currentRoom
Returns    - N/A
*/
void fetchContents(FILE *theFile, GameState *theGame, int currentRoom) {

    // Instantiate buffer
    int bufferIndex = 0;
    char *buffer = (char*)malloc(sizeof(char) * 128);

    // For each character in the file
    int i;
    while (( i = fgetc(theFile)) != EOF ) {

        // If its the end of the line..
        if (i == '\n') {

            // Store the parsed strings into a key value pair sort of...
            // Got the influence from some of the comments here:
            // http://stackoverflow.com/questions/2663644/parse-values-from-a-text-file-in-c
            char *keyIndex = (char*)malloc(sizeof(char) * 60);
            sscanf(buffer, "%s %s", keyIndex, keyIndex);

            char *valueIndex = (char*)malloc(sizeof(char) * 60);
            sscanf(buffer, "%s %s %s", valueIndex, valueIndex, valueIndex);

            // See if we have any key value matches
            checkMatch(theFile, theGame, currentRoom, keyIndex, valueIndex);

            // Add terminator to string
            buffer[bufferIndex] = '\0';
            bufferIndex = 0;
        } else {

            // Increment to next line please!
            buffer[bufferIndex] = i;
            bufferIndex++;
        }
    }
}

/*
Function   - checkMatch(): Helper helper function, Parses file for room details
Parameters - FILE, GameState, currentRoom, keyIndex, valueIndex
Returns    - N/A
*/
void checkMatch(FILE *theFile, GameState *theGame, int currentRoom, char *keyIndex, char *valueIndex) {
    char* connectionList[] = {"0:", "1:", "2:", "3:", "4:" "5:"};

    // Check for roomName match
    if (strcmp(keyIndex, "NAME:") == false) {
        theGame[currentRoom].roomName = valueIndex;
    }

    // Check for connection match
    int i=0;
    for (i = 0; i < 5; i++) {

        // If connection matches, add it
        if (strcmp(keyIndex, connectionList[i]) == false) {
            theGame[currentRoom].connections[theGame[currentRoom].totalConnections] = valueIndex;
            theGame[currentRoom].totalConnections++;
        }
    }

    // Check for roomType match
    if (strcmp(keyIndex, "TYPE:") == false) {
        if (strcmp(valueIndex, "END_ROOM") == false) {
            theGame[currentRoom].type = END_ROOM;
        } else if (strcmp(valueIndex, "START_ROOM") == false) {
            theGame[currentRoom].type = START_ROOM;
        } else {
            theGame[currentRoom].type = MID_ROOM;
        }
    }
}

/*
Function   - gameLoop(): Main game loop, interacts with user and performs actions.
Parameters - currentRoom, endRoom, GameState
Returns    - N/A
*/
void gameLoop(int currentRoom, int endRoom, GameState *roomList) {
    // Initiate variables for user input and path history
    int totalSteps = 0;
    char **userPath = (char **)malloc(sizeof(char *) * 255);
    char input[50];

    // The main user input loop
    while (endRoom != currentRoom) {

        // Output visible rooms
        printf("CURRENT LOCATION: %s\n", roomList[currentRoom].roomName);
        printf("POSSIBLE CONNECTIONS: ");

        int i;
        for (i = 0; i < roomList[currentRoom].totalConnections; i++) {
            printf("%s", roomList[currentRoom].connections[i]);

            if (roomList[currentRoom].totalConnections > (i + 1)) {
                printf(", ");
            } else {
                printf(".\n");
            }
        }

        // Prompt for input
        printf("WHERE TO? >");
        scanf("%s", &input);

        // Error check user
        if (checkConnection(input, roomList, currentRoom) == false) {
            printf("\nHUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
        } else {

            // Move room
            for (i = 0; i < 7; i++) {
                if (strcmp(roomList[i].roomName, input) == false) {
                    currentRoom = i;
                }
            }

            // Update user path
            userPath[totalSteps] = (char*)malloc(sizeof(char) * strlen(input));
            strcpy(userPath[totalSteps], input);

            // End and increment
            printf("\n");
            totalSteps++;
        }
    }

    // End the game
    endMessage(userPath, totalSteps);
}

/*
Function   - checkConnection(): Checks if there's a connection between two rooms
Parameters - roomName, GameState, room
Returns    - bool
*/
bool checkConnection(char* roomName, GameState* array, int room1) {
    int room2 = -1;

    int i;
    for (i = 0; i < 7; i++) {
        if (strcmp(roomName, array[i].roomName) == false) {
            room2 = i;
            break;
        }
    }

    if (room2 == -1) {
        return false;
    }

    for (i = 0; i < array[room1].totalConnections; i++) {
        if (strcmp(array[room1].connections[i], array[room2].roomName) == false) {
            return true;
        }
    }

    return false;
}

/*
Function   - isValueInArray(): Checks if value is found in array
Parameters - value, array, size
Returns    - Boolean
*/
bool isValueInArray(int value, int *array, int size) {
    int i;
    // For every index in array
    for (i = 0; i < size; i++) {

        // Does it exist?
        if (array[i] == value) {
            return true;
        }
    }

    return false;
}

/*
Function   - endMessage(): Prints message when game is over
Parameters - userPath, totalSteps
Returns    - N/A
*/
void endMessage(char **userPath, int totalSteps) {
    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", totalSteps);
    int i;
    for (i = 0; i < totalSteps; i++) {
        printf("%s\n", userPath[i]);
    }
}

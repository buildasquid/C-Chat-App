#include <stdbool.h>       //  using bool data type and true/false constants
#include <unistd.h>        //  system calls like close()
#include <pthread.h>       //  multithreading support (pthread_create, etc.)
#include <stdio.h>         //  standard I/O functions (printf, getline)
#include <stdlib.h>        //  memory allocation (malloc) and general utilities
#include <string.h>        //  string manipulation (strcmp, sprintf)
#include <sys/socket.h>    //  socket programming (socket, send, recv)
#include <netinet/in.h>    //  defining internet protocols (IPv4 addresses)
#include <arpa/inet.h>     //  functions like inet_pton to handle IP address conversion
#include <stdint.h>        //  using integer types with defined sizes (intptr_t)


// prototypes
void startListeningAndPrintMessagesOnNewThread(int fd);
void listenAndPrint(void *arg);
void readConsoleEntriesAndSendToServer(int socketFD);
int createTCPIpv4Socket();
struct sockaddr_in *createIPv4Address(const char *ip, int port);

int main() {
    // create a socket for TCP IPv4 communication
    int socketFD = createTCPIpv4Socket();

    // create an IPv4 address structure with IP "127.0.0.1" (localhost) and port 2000
    struct sockaddr_in *address = createIPv4Address("127.0.0.1", 2000);

    // connect to the server using the created socket and address
    int result = connect(socketFD, (struct sockaddr *)address, sizeof(*address));

    // check if the connection was successful
    if(result == 0)
        printf("connection was successful!!\n");

    // start a new thread to listen for incoming messages from the server
    startListeningAndPrintMessagesOnNewThread(socketFD);

    // read input from the user and send it to the server
    readConsoleEntriesAndSendToServer(socketFD);

    //close the socket after use
    close(socketFD);

    return 0;
}

// function to read console input and send it to the server
void readConsoleEntriesAndSendToServer(int socketFD) {
    // allocate memory for the name and get input from the user
    char *name = NULL;
    size_t nameSize = 0;
    printf("choose a display name:\n");
    ssize_t nameCount = getline(&name, &nameSize, stdin);

    
    name[nameCount - 1] = 0;     // remove the newline character from the name
    printf("congrats! your name is now: %s\n", name);
    // allocate memory for each line of input
    char *line = NULL;
    size_t lineSize = 0;
    printf("type a message and hit enter(or type 'exit' to quit)\n");

    // buffer to store the formatted message
    char buffer[1024];

    // start a loop to continuously get user input and send to the server
    while (true) {
        // read a line from console input
        ssize_t charCount = getline(&line, &lineSize, stdin);

        line[charCount - 1] = 0; // remove the newline character from the input

        // format the message to be sent (name:message)
        sprintf(buffer, "%s:%s", name, line);

        // if user types 'exit' break the loop and stop sending messages
        if (charCount > 0) {
            if (strcmp(line, "exit") == 0)
                break;

            // send the formatted message to server
            ssize_t amountWasSent = send(socketFD, buffer, strlen(buffer), 0);
        }
    }
}

// function: start a new thread that listens and prints messages from the server
void startListeningAndPrintMessagesOnNewThread(int socketFD) {
    // thread id
    pthread_t id;

    // create a new thread to run listenAndPrint, passing socketFD as an argument
    pthread_create(&id, NULL, (void *(*)(void *))listenAndPrint, (void *)(intptr_t)socketFD);
}

// function: listen for messages from the server and print them to the console
void listenAndPrint(void *arg) {
    // convert the argument back to socketFD (intptr_t -> int)
    int socketFD = (intptr_t)arg;

    // buffer to store received messages
    char buffer[1024];

    // loop to continuously listen for server responses
    while (true) {
        // receive data from the server (up to 1024 bytes)
        ssize_t amountReceived = recv(socketFD, buffer, 1024, 0);

        // if data is received, print it
        if (amountReceived > 0) {
            buffer[amountReceived] = 0;  // null-terminate the received string
            printf("reply: %s\n", buffer);
        }

        // if no data is received break the loop
        if (amountReceived == 0)
            break;
    }

    // close the socket after receiving data
    close(socketFD);
}

// function: create a TCP IPv4 socket
int createTCPIpv4Socket() {
    // create a TCP socket (IPv4) and return its file descriptor
    return socket(AF_INET, SOCK_STREAM, 0);
}

// function: create an IPv4 address structure with the given IP and port
struct sockaddr_in *createIPv4Address(const char *ip, int port) {

    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));

    address->sin_family = AF_INET;// IPv4 address fam

    // set the port number (in correct byte order)
    address->sin_port = htons(port);

    // convert IP address from string to binary form
    inet_pton(AF_INET, ip, &address->sin_addr.s_addr);


    return address;
}

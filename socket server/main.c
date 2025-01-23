#include <stdbool.h>       //  using bool data type and true/false constants
#include <unistd.h>        //  system calls like close()
#include <pthread.h>       //  multithreading support (pthread_create, etc.)
#include <stdio.h>         //  standard I/O functions (printf)
#include <stdlib.h>        //  memory allocation (malloc) and general utilities
#include <string.h>        //  string manipulation (strlen)
#include <sys/socket.h>    //  socket programming (socket, send, recv)
#include <netinet/in.h>    //  defining internet protocols (IPv4 addresses)
#include <arpa/inet.h>     //  functions like inet_pton to handle IP address conversion
#include <stdint.h>        //  using integer types with defined sizes (intptr_t)


// structure to store information about an accepted socket connection
struct AcceptedSocket {
    int acceptedSocketFD;     // socket file descriptor for the accepted connection
    struct sockaddr_in address; // address of the accepted client
    int error;                // stores the error code if the acceptance fails
    bool acceptedSuccessfully; // flag to indicate whether the connection was successful
};

// function Prototypes
struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD);
void acceptNewConnectionAndReceiveAndPrintItsData(int serverSocketFD);
void receiveAndPrintIncomingData(int socketFD);
void startAcceptingIncomingConnections(int serverSocketFD);
void receiveAndPrintIncomingDataOnSeparateThread(struct AcceptedSocket *pSocket);
void sendReceivedMessageToTheOtherClients(char *buffer, int socketFD);

// array to store accepted socket connections
struct AcceptedSocket acceptedSockets[10];
int acceptedSocketsCount = 0; // Counter to track the number of accepted connections

// function to start accepting incoming connections
void startAcceptingIncomingConnections(int serverSocketFD) {
    while (true) {
        // accept a new incoming connection and store it in the acceptedSockets array
        struct AcceptedSocket *clientSocket = acceptIncomingConnection(serverSocketFD);
        acceptedSockets[acceptedSocketsCount++] = *clientSocket;

        // start a separate thread to handle receiving and printing data from the client
        receiveAndPrintIncomingDataOnSeparateThread(clientSocket);
    }
}

// function to create a new thread to handle receiving and printing incoming data
void receiveAndPrintIncomingDataOnSeparateThread(struct AcceptedSocket *pSocket) {
    pthread_t id; // thread id
    // create a new thread and pass the socket file descriptor to receive and print data
    pthread_create(&id, NULL, (void *(*)(void *))receiveAndPrintIncomingData, (void *)(intptr_t)pSocket->acceptedSocketFD);
}

// function to receive and print incoming data from a client
void receiveAndPrintIncomingData(int socketFD) {
    char buffer[1024]; // buffer to store the received message

    while (true) {
        // receive data from the client (up to 1024 bytes)
        ssize_t amountReceived = recv(socketFD, buffer, 1024, 0);

        // if data is received, print it
        if (amountReceived > 0) {
            buffer[amountReceived] = 0; // null-terminate the received string
            printf("%s\n", buffer);

            // send the received message to the other connected clients
            sendReceivedMessageToTheOtherClients(buffer, socketFD);
        }

        // if no data is received (amountReceived == 0), break the loop
        if (amountReceived == 0)
            break;
    }

    // close the socket after the communication is complete
    close(socketFD);
}

// function to send the received message to all other clients (except the sender)
void sendReceivedMessageToTheOtherClients(char *buffer, int socketFD) {
    // loop through all accepted sockets and send the message to clients that are not the sender
    for (int i = 0; i < acceptedSocketsCount; i++)
        if (acceptedSockets[i].acceptedSocketFD != socketFD) {
            // send the message to the other clients
            send(acceptedSockets[i].acceptedSocketFD, buffer, strlen(buffer), 0);
        }
}

// function to accept an incoming connection and return a structure with the socket details
struct AcceptedSocket *acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in clientAddress; // client's address information
    int clientAddressSize = sizeof(struct sockaddr_in);
    
    // accept an incoming connection from the server socket
    int clientSocketFD = accept(serverSocketFD, (struct sockaddr *)&clientAddress, (socklen_t *)&clientAddressSize);

    // allocate memory for an AcceptedSocket structure
    struct AcceptedSocket *acceptedSocket = malloc(sizeof(struct AcceptedSocket));
    acceptedSocket->address = clientAddress; // store the client address
    acceptedSocket->acceptedSocketFD = clientSocketFD; // store the accepted socket file descriptor
    acceptedSocket->acceptedSuccessfully = clientSocketFD > 0; // flag indicating success

    // if the connection was not successful, store the error code
    if (!acceptedSocket->acceptedSuccessfully)
        acceptedSocket->error = clientSocketFD;

    return acceptedSocket; 
}

// function to create a TCP IPv4 socket
int createTCPIpv4Socket() {
    // create a TCP socket using IPv4 address family and return the socket file descriptor
    return socket(AF_INET, SOCK_STREAM, 0);
}

// function to create an IPv4 address structure for the given IP and port
struct sockaddr_in *createIPv4Address(const char *ip, int port) {
    // allocate memory for the sockaddr_in structure
    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
    
    // set the address family to IPv4
    address->sin_family = AF_INET;

    // set the port number (convert it to network byte order)
    address->sin_port = htons(port);

    // convert the IP address from string to binary form and store it in sin_addr
    inet_pton(AF_INET, ip, &address->sin_addr.s_addr);

    return address;
}


int main() {
    // create a server socket using TCP IPv4
    int serverSocketFD = createTCPIpv4Socket();

    // create the server's IPv4 address (bind to all available interfaces, port 2000)
    struct sockaddr_in *serverAddress = createIPv4Address("", 2000);

    // bind the server socket to the address and port
    int result = bind(serverSocketFD, (const struct sockaddr *)serverAddress, sizeof(*serverAddress));
    
    
    if (result == 0)
        printf("socket bound successfully!!\n");

    int listenResult = listen(serverSocketFD, 10);
    startAcceptingIncomingConnections(serverSocketFD);
    shutdown(serverSocketFD, SHUT_RDWR);

    return 0; 
}

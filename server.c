#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


static const int maxConnections = 5; 
int sizeBuffer;
char * uDirectory;


void error(int errorCode, const char* responseMessage)  {
    printf("Error Code: %d - Response Message: %s\n", errorCode, responseMessage);
    exit(1);
}

void get(int clientSocket, const char *fileName) {
    int uFile;
    ssize_t readBytes;
    char * uBuffer = (char *) malloc (sizeof(char) * sizeBuffer);
    char *filePath = (char *) malloc (sizeof(char) * sizeBuffer);
    strcpy(filePath, uDirectory);
    strcat(filePath, fileName);

    uFile = open(filePath, O_RDONLY);
    if (uFile < 0 && uFile > -2) {
        error(-8, "File not found");
    }

    readBytes = read(uFile, uBuffer, sizeBuffer);
    while (readBytes > 0) {
        if (write(clientSocket, uBuffer, sizeBuffer) == -1) {
            error(-7, "Fail to connect client-server");
        }

        readBytes = read(uFile, uBuffer, sizeBuffer);
    }

    if (readBytes < 0 && readBytes > -2) {
        error(-12, "Fail to R/W");
    }

    close(uFile);
}

void list(int clientSocket) {
    struct dirent *file;
    char * uBuffer = (char *) malloc (sizeof(char) * sizeBuffer);
    DIR *dir = opendir(uDirectory);
    if (dir) {
        while ((file = readdir(dir)) != NULL) {
            if (file->d_type == DT_REG) {
                sprintf(uBuffer, "%s", file->d_name);
                send(clientSocket, uBuffer, sizeBuffer, 0);
            }
        }
        closedir(dir);
    }
}

void* handleConnection(void* arg) {
    int clientSocket = (intptr_t) arg;
    char* uBuffer = (char *) malloc(sizeof(char) *sizeBuffer);
    char* op = (char*) malloc(sizeof(char) * sizeBuffer);

    ssize_t numBytesRcvd = recv(clientSocket, uBuffer, sizeBuffer, 0);

    if (numBytesRcvd < 0) {
        error(-7, "Fail to connect client-server");
    }
    sscanf(uBuffer, "%s", op);

    uBuffer = uBuffer + strlen(op); 
    if (strcmp(op, "list") == 0) {
        list(clientSocket);
    } else if (strcmp(op, "get") == 0) {
        char *fileName = (char*) malloc(sizeof(char) * sizeBuffer);
        sscanf(uBuffer, "%s", fileName);
        get(clientSocket, fileName);
    } else {
        error(-10, "Client command not found");
    }

    close(clientSocket);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])  {
    if (argc != 4 ) {
        error(-1, "Erros nos argumentos de entrada");
    }

    in_port_t portServer = atoi(argv[1]); // First arg: local port
    sizeBuffer = atoi(argv[2]);
    uDirectory = argv[3];

    pthread_t threadClient;

    int servSock;
    if ((servSock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0){
        error(-2, "Fail to create socket");
    }

    struct sockaddr_in6 serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin6_family = AF_INET6;
    inet_pton(AF_INET6, in6addr_loopback.s6_addr, &(serverAddress.sin6_addr.s6_addr));
    serverAddress.sin6_port = htons(portServer);

    if (bind(servSock, (struct socketAddress*) &serverAddress, sizeof(serverAddress)) < 0){
        error(-3, "Bind error");
    }

    if (listen(servSock, maxConnections) < 0){
        error(-4, "Listen error");
    }else{
        printf("Server started. \nWaiting conections...\n");
    }

    while (1){
        struct sockaddr_in6 clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);

        int clientSocket = accept(servSock, (struct socketAddress *) &clientAddress, &clientAddressLen);
        if (clientSocket >= 0){
            printf("Accept() - Sucess!\n");
        }else{
            error(-5, "Accept error");
        }

        char clntName[INET6_ADDRSTRLEN];

        if (inet_ntop(AF_INET6, &clientAddress.sin6_addr.s6_addr, clntName, sizeof(clntName)) != NULL)
            printf("IP/Port: %s:%d\n", clntName, ntohs(clientAddress.sin6_port));
        else
            error(-11, "Client Address not found");

        pthread_create(&threadClient, NULL, handleConnection, (void *) (intptr_t) clientSocket);
    }
}

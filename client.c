#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int sizeBuffer;

void error(int errorCode, const char* responseMessage)  {
    printf("Error Code: %d - Response Message: %s\n", errorCode, responseMessage);
    exit(1);
}

void list(int uSocket) {
    char uBuffer[sizeBuffer];

    ssize_t bytesRec = recv(uSocket, uBuffer, sizeBuffer, 0);
    while (bytesRec > 0) {
        printf("%s\n", uBuffer);
        bytesRec = recv(uSocket, uBuffer, sizeBuffer, 0);
      }
}

void get(int uSocket, const char *fileName) {
    int uFile;
    ssize_t read_return;
    if((uFile = open(fileName, O_CREAT|O_WRONLY, 0777)) == -1) {
        error(-13, "Fail to create file");
    }

    char uBuffer[sizeBuffer];

    struct timeval t1, t2;
    double elapsedTime;

    gettimeofday(&t1, NULL);
    ssize_t readBytes = read(uSocket, uBuffer, sizeBuffer);
    while (readBytes > 0) {
        if (write(uFile, uBuffer, sizeBuffer) == -1) {
            error(-12, "Fail to R/W");
        }
        readBytes = read(uSocket, uBuffer, sizeBuffer);
    }

    if (readBytes < 0) {
        error(-7, "Fail to connect client-server");
    }

    close(uFile);
    gettimeofday(&t2, NULL);

    //milliseconds convertion
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   
    printf("Received (Total: %.2f milliseconds!\n", elapsedTime);
}

int main(int argc, char *argv[]) {

    if (argc != 5 && argc != 6) {
        error(-1, "Input error");
    }

    char *command = argv[1];
    char *server;
    char *fileName;
    in_port_t serverPort;
    char* message = (char *) malloc(sizeof(char) * 256);

    if (strcmp(command, "list") == 0) {
        server = argv[2];
        serverPort = atoi(argv[3]);
        sizeBuffer = atoi(argv[4]);
    } else if (strcmp(command, "get") == 0) {
        fileName = argv[2];
        server = argv[3];
        serverPort = atoi(argv[4]);
        sizeBuffer = atoi(argv[5]);
    } else {
        error(-10, "Client command not found");
    }

    int uSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if (uSocket < 0){
        error(-2, "Fail to create socket");
    }

    struct socketAddress_in6 serverAddress; 

    memset(&serverAddress, 0, sizeof(serverAddress)); 
    serverAddress.sin6_family = AF_INET6;

    int rtnVal = inet_pton(AF_INET6, server, &serverAddress.sin6_addr.s6_addr);

    if (rtnVal < 0) {
        error(-101, "Fail to Address convertion");
    }
    else if (rtnVal == 0) {
        error(-100, "invalid Address");
    }

    serverAddress.sin6_port = htons(serverPort); 

    if (connect(uSocket, (struct socketAddress *) &serverAddress, sizeof(serverAddress)) < 0){
        error(-6, "Connection Error");
    }

    sprintf(message, "%s %s", command, fileName);
    size_t messageLen = strlen(message);

    ssize_t numBytes = send(uSocket, message, messageLen, 0);

    if (numBytes >= 0)
        printf("send() - Sending message\n");
    else{
        error(-7, "Fail to connect client-server\n");
    }

    if (strcmp(command, "get") == 0) {
        get(uSocket, fileName);
    } else if (strcmp(command, "list") == 0) {
        list(uSocket);
    }

    close(uSocket);
    exit(0);
}

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

int sizeBuff;

void error( const char* errMsg)  {
    printf("Error: %s\n", errMsg);
    exit(1);
}

void list(int sock) {
    char buffer[sizeBuff];
    ssize_t bytesRec = recv(sock, buffer, sizeBuff, 0);
    while (bytesRec > 0) {
        printf("%s\n", buffer);
        bytesRec = recv(sock, buffer, sizeBuff, 0);
      }
}

void get(int sock, const char *fileName) {
    int arq;
    ssize_t read_return;
    if((arq = open(fileName, O_CREAT|O_WRONLY, 0777)) == -1) {
        error("Error to create file");
    }

    char buffer[sizeBuff];
    struct timeval t1, t2;
    double elapsedTime;

    gettimeofday(&t1, NULL);
    ssize_t readBytes = read(sock, buffer, sizeBuff);
    while (readBytes > 0) {
        if (write(arq, buffer, sizeBuff) == -1) {
            error("Error to R/W file");
        }
        readBytes = read(sock, buffer, sizeBuff);
    }

    if (readBytes < 0) {
        error( "Error linking client/server");
    }

    close(arq);
    gettimeofday(&t2, NULL);

    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;   
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   
    printf("Sucess! (Total time: %.2f ms!\n", elapsedTime);
}

int main(int argc, char *argv[]) {

    if (argc != 5 && argc != 6) {
        error( "Argc is not 5 or 6 - Error");
    }

    char *command = argv[1];
    char *server;
    char *fileName;
    in_port_t serverPort;
    char* message = (char *) malloc(sizeof(char) * 256);


    if (strcmp(command, "get") == 0) {
        fileName = argv[2];
        server = argv[3];
        serverPort = atoi(argv[4]);
        sizeBuff = atoi(argv[5]);
    }
    else if (strcmp(command, "list") == 0) {
        server = argv[2];
        serverPort = atoi(argv[3]);
        sizeBuff = atoi(argv[4]);
    }  else {
        error( "Client command not exists");
    }

    int sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if (sock < 0){
        error( "Error to create socket");
    }

    struct sockaddr_in6 servAddr;

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin6_family = AF_INET6;
    int rtnVal = inet_pton(AF_INET6, server, &servAddr.sin6_addr.s6_addr); //conversão do endereço de txt pra binary

    if (rtnVal < 0) {
        error( "Fail to convert address");
    }
    else if (rtnVal == 0) {
        error( "Invalid Address");
    }

    servAddr.sin6_port = htons(serverPort); //porta do servidor
    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){  // conectando ao servidor
        error( "Erro de connect");
    }

    sprintf(message, "%s %s", command, fileName);
    size_t messageLen = strlen(message);
    ssize_t numBytes = send(sock, message, messageLen, 0);

    if (numBytes < 1 && numBytes != 0)
        error( "Error linking client/server\n");
    else{
        printf("Sending message to server...\n");
    }	

    if (strcmp(command, "list") == 0) {
        list(sock);
    } else if (strcmp(command, "get") == 0) {
        get(sock, fileName);
    }

    close(sock);
    exit(0);
}


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


int sizeBuff;
char * pathDir;
static const int maxRequests = 5; 

void error(const char* errMsg)  {
    printf("Error: %s\n", errMsg);
    exit(1);
}

void get(int clientSocket, const char *fileName) {
    int arq;
    ssize_t readBytes;
    char * buffer = (char *) malloc (sizeof(char) * sizeBuff);
    char *filePath = (char *) malloc (sizeof(char) * sizeBuff);
    strcpy(filePath, pathDir);
    strcat(filePath, fileName);

    arq = open(filePath, O_RDONLY);
    if (arq < 0 && arq> -2) { // ou seja, não achou o arquivo para leitura
        error("File not found");
    }

    readBytes = read(arq, buffer, sizeBuff);
    while (readBytes > 0) {
        if (write(clientSocket, buffer, sizeBuff) == -1) { 
            error( "Fail in client/server link");
        }

        readBytes = read(arq, buffer, sizeBuff);
    }

    if (readBytes< 0 && readBytes> -2) {
        error("Error file R/W");
    }

    close(arq);
}

void list(int clientSocket) {
    struct dirent *file;
    char * buffer = (char *) malloc (sizeof(char) * sizeBuff);
    DIR *dir = opendir(pathDir);
    if (dir) {
        while ((file = readdir(dir)) != NULL) {
            if (file->d_type == DT_REG) {
                sprintf(buffer, "%s", file->d_name);
                send(clientSocket, buffer, sizeBuff, 0);
            }
        }
        closedir(dir);
    }
}

void* handleConnection(void* arg) {
    int clientSocket = (intptr_t) arg;
    char* buffer = (char *) malloc(sizeof(char) *sizeBuff);
    char* op = (char*) malloc(sizeof(char) * sizeBuff);
    ssize_t numBytesRcvd = recv(clientSocket, buffer, sizeBuff, 0);

    if (numBytesRcvd < -1 || numBytesRcvd == 0) {
        error( "Error linking");
    }
    sscanf(buffer, "%s", op);

    buffer = buffer + strlen(op);
   	if (strcmp(op, "get") == 0) {
        char *fileName = (char*) malloc(sizeof(char) * sizeBuff);
        sscanf(buffer, "%s", fileName);
        get(clientSocket, fileName);
    } 
	else if (strcmp(op, "list") == 0) {
        list(clientSocket);
	}
	else {
        error( "Client command not exists");
    }

    close(clientSocket);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])  {
    if (argc != 4 ) {
        error( "Argc is not 4 - Error");
    }

    in_port_t portServer = atoi(argv[1]);
    sizeBuff = atoi(argv[2]);
    pathDir = argv[3];
    pthread_t threadClient;

    int servSock;
    if ((servSock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0){
        error("Error to create socket");
    }

    struct sockaddr_in6 servAddr;
    memset(&servAddr, 0, sizeof(servAddr));

    servAddr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, in6addr_loopback.s6_addr, &(servAddr.sin6_addr.s6_addr));
    servAddr.sin6_port = htons(portServer);

    if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0){
        error( "Bind error");
    }

    if (listen(servSock, maxRequests) < 0){
        error( "Listen error");
    }else{
        printf("Initializing server. \nWaiting connections..\n");
    }

    while (1){
        struct sockaddr_in6 clntAddr;
        socklen_t clntAddrLen = sizeof(clntAddr);

        int clientSocket = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);
        if (clientSocket < 0){
            error("Erro de accept");
        }else{
            printf("Accept() - Conexão recebida e aceita!\n");
        }

        char clntName[INET6_ADDRSTRLEN];

        if (inet_ntop(AF_INET6, &clntAddr.sin6_addr.s6_addr, clntName, sizeof(clntName)) != NULL)
            printf("IP/Port: %s:%d\n", clntName, ntohs(clntAddr.sin6_port));
        else
            error( "Endereco do cliente nao encontrado");

        pthread_create(&threadClient, NULL, handleConnection, (void *) (intptr_t) clientSocket);
    }
}

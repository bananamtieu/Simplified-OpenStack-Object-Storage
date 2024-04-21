/*
Chan Nam Tieu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include <math.h>
#include "preprocess.h"

#define MAX_LOGIN_NAME 20
#define MAX_NUMDISKS 10
#define MAX_IPLENGTH 20
#define MAX_NUMFILES 100
#define MAX_PATHLENGTH 40

struct disk {
    char diskIp[MAX_IPLENGTH];
    int numFiles;
    char fileList[MAX_NUMFILES][MAX_PATHLENGTH];
};

typedef struct disk disk;

int numDisks = 0;
disk DiskList[MAX_NUMDISKS];

int getDiskIndex(char *diskIp) {
    for (int i = 0; i < numDisks; i++) {
        if (strcmp(DiskList[i].diskIp, diskIp) == 0) {
            return i;
        }
    }
    return -1;
}

void _upload(char* userFilename, int socket) { //, int partition, disk *DiskList, char d, char *partitionArray, char *userFileHashSet) {
    char username[10], filename[10], tempFilename[20];
    strcpy(tempFilename, userFilename);
    char *token = strtok(tempFilename, "/");
    if (token != NULL) {
        strcpy(username, token);
        token = strtok(NULL, "/");
        if (token != NULL) {
            strcpy(filename, token);
        } else {
            printf("Error: Missing filename\n");
            return;
        }
    }

    const char* directory = "/tmp/tempFile/";
    struct stat sb;
    if (stat(directory, &sb) != 0) {
        printf("Temporary directory does not exist. Creating %s\n", directory);
        char command[100];
        strcpy(command, "mkdir -p /tmp/tempFile/");
        if (system(command) != 0) {
            printf("Error: Failed to create directory.\n");
            return;
        }
    }
    else {
        printf("Temporary directory exists.\n");
    }

    char filePath[50];
    strcpy(filePath, directory);
    strcat(filePath, filename);

    FILE *fileToUpload = fopen(filePath, "w");
    char buff[1024];
    int bytesRead;
    printf("File directory is %s\n", filePath);
    while ((bytesRead = read(socket, buff, 1024)) > 0) {
        fwrite(buff, sizeof(char), bytesRead, fileToUpload);
    }
    fclose(fileToUpload);
    printf("Finished uploading file to server.\n");

    char *userFileHash = md5_hash(userFilename);
    
}

int main(int argc, char *argv[]){

    //Declare socket file descriptor.
    int sockfd, connfd;

    //Declare server address to which to bind for receiving messages and client address to fill in sending address
    struct sockaddr_in servAddr, clienAddr;

    //Get from the command line, server IP, src and dst files.
    if (argc != 2){
        printf ("Usage: %s <# partition power> <available disks> \n",argv[0]);
        exit(0);
    }

    int partition = atoi(argv[1]);
    numDisks = argc - 2;

    for (int i = 0; i < numDisks; i++) {
        strcpy(DiskList[i].diskIp, argv[i + 2]);
    }

    char login_name[MAX_LOGIN_NAME];
    getlogin_r(login_name, MAX_LOGIN_NAME);

    int numPartition = (int) pow(2, partition);
    int partitionArray[numPartition];
    char DPAHelper[numPartition][MAX_PATHLENGTH];
    for (int i = 0; i < numPartition; i++) {
        partitionArray[i] = 0;
        strcpy(DPAHelper[i], "");
    }

    for (int pNum = 0; pNum < numPartition; pNum++) {
        for (int i = 0; i < numDisks; i++) {
            if (pNum >= numPartition*i/numDisks && pNum < numPartition*(i + 1)/numDisks) {
                printf("%d,", pNum);
                partitionArray[pNum] = i;
            }
        }
    }

    //Open a TCP socket, if successful, returns a descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("cannot create socket");
        exit(0);
    }

    int bindFail = 1;
    int portNumber;
    srand(time(0));
    while (bindFail) {
        portNumber = rand()%8400 + 1024;
        //Setup the server address to bind using socket addressing structure
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(portNumber);
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        //bind IP address and port for server endpoint socket
        if ((bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr))) < 0){
            perror("Failure to bind server address to the endpoint socket");
            exit(0);
        }
        else {
            bindFail = 0;
        }
    }

    // Server listening to the socket endpoint, and can queue 5 client requests
    printf("Server listening/waiting for client at port %d\n", portNumber);
    listen(sockfd, 5);


    //Server accepts the connection and call the connection handler
    size_t sin_size = sizeof(clienAddr);
    char buff[1024];
    char cmdInput[30], command[10], arg[20];
    while(1) {
        if ((connfd = accept(sockfd, (struct sockaddr *)&clienAddr, (socklen_t *)&sin_size)) < 0){
            perror("Failure to accept connection to the client");
            exit(0);
        }
        printf("Connection Established with client IP: %s and Port: %d\n", inet_ntoa(clienAddr.sin_addr), ntohs(clienAddr.sin_port));
        ssize_t bytes_read = read(connfd, buff, sizeof(buff) - 1); // -1 to leave space for null terminator
        if (bytes_read < 0) {
            perror("Error reading from socket");
            exit(1);
        }
        // Ensure cmdInput is properly null-terminated
        buff[bytes_read] = '\0'; // Null-terminate the buffer
        strcpy(cmdInput, buff); // Copy buffer to cmdInput

        char *token = strtok(cmdInput, " ");
        if (token != NULL) {
            //printf("Token 1: %s\n", token); // Debug print
            strcpy(command, token);
            token = strtok(NULL, " ");
            if (token != NULL) {
                //printf("Token 2: %s\n", token); // Debug print
                strcpy(arg, token);
            }
        }
        printf("userFilename = %s\n", arg); // Ensure newline to flush output



        if (strcmp(command, "upload") == 0) {
            _upload(arg, connfd);
        }
        else if (strcmp(command, "download") == 0) {
            //_download(arg, sockfd);
        }
        else if (strcmp(command, "list") == 0) {
            //_list(arg, sockfd);
 
/*
Chan Nam Tieu
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>

void _upload(char* userFilename, int socket) {
    char username[10], filename[10];
    char *token = strtok(userFilename, "/");
    strcpy(username, token);
    token = strtok(userFilename, "/");
    strcpy(filename, token);

    FILE *fileToUpload = fopen(filename, "r");
    char buff[1024];
    if (fileToUpload != NULL) {
        int bytesRead;
        while ((bytesRead = fread(buff, sizeof(char), sizeof(buff), fileToUpload)) > 0) {
            write(socket, buff, bytesRead);
        }
        fclose(fileToUpload);
    }
    else {
        printf("File does not exist: %s\n", filename);
    }
    read(socket, buff, 1024);
    printf("Server's message: %s\n", buff);
}

void _download(char* userFilename, int socket) {
    char buff[1024];
    // Receving message from server
    read(socket, buff, 1024);
    if (strcmp(buff, "The file is not exist.") == 0) {
        printf("%s\n", buff);
    }

    char username[10], filename[10];
    char *token = strtok(userFilename, "/");
    strcpy(username, token);
    token = strtok(userFilename, "/");
    strcpy(filename, token);

    FILE *fileToDownload = fopen(filename, "w");
    if (strcmp(filename, "q") != 0) {
        int bytesRead;
        while ((bytesRead = read(socket, buff, 1024)) > 0) {
            fwrite(buff, sizeof(char), bytesRead, fileToDownload);
        }
        fclose(fileToDownload);
    }

}

void _list(char* userFilename, int socket) {
    char username[10], filename[10];
    char *token = strtok(userFilename, "/");
    strcpy(username, token);
    token = strtok(userFilename, "/");
    strcpy(filename, token);

    FILE *outputFile = fopen("output.txt", "w");
    char buff[1024];
    int bytesRead;
    while ((bytesRead = read(socket, buff, 1024)) > 0) {
        fwrite(buff, sizeof(char), bytesRead, outputFile);
    }
    fclose(outputFile);
}

void _delete(char* userFilename, int socket) {
    char buff[1024];
    // Receving message from server
    read(socket, buff, 1024);
    if (strcmp(buff, "The file is not exist.") == 0) {
        printf("%s\n", buff);
    }

    read(socket, buff, 1024);
    printf("%s\n", buff);
}

void _add(char* userFilename, int socket) {
    char buff[1024];
    read(socket, buff, 1024);
    printf("Server's message: %s\n", buff);
}

void _remove(char* userFilename, int socket) {
    char buff[1024];
    read(socket, buff, 1024);
    printf("Server's message: %s\n", buff);
}

int main(int argc, char *argv[]){
    //Get server IP and port number from command line
    if (argc != 3){
		printf ("Usage: %s <ip of server> <port #>\n",argv[0]);
		exit(0);
    }
    //Declare socket file descriptor and buffer
    int sockfd;
    char input[30], command[10], arg[20];

    //Declare server address to accept
    struct sockaddr_in servAddr;

   //Declare host
    struct hostent *host;

    //get hostname
    host = (struct hostent *)gethostbyname(argv[1]);

    //Open a socket, if successful, returns
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failure to setup an endpoint socket");
        return 0;
    }

    //Set the server address to send using socket addressing structure
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(atoi(argv[2]));
    servAddr.sin_addr = *((struct in_addr *)host->h_addr);

    //Connect to the server
    if (connect(sockfd, (struct sockaddr *)&servAddr, sizeof(struct sockaddr)) < 0){
        perror("Failure to connect to the server");
        exit(1);
    }

    // Prompt user to enter command
    printf("Please enter a command: ");
    scanf("%s", input);
    printf("%s", input);

    write(sockfd, input, sizeof(input));

    char *token = strtok(input, " ");
    strcpy(command, token);
    token = strtok(input, " ");
    strcpy(arg, token);

    if (strcmp(command, "upload") == 0) {
        _upload(arg, sockfd);
    }
    if (strcmp(command, "download") == 0) {
        _download(arg, sockfd);
    }
    if (strcmp(command, "list") == 0) {
        _list(arg, sockfd);
    }
    if (strcmp(command, "delete") == 0) {
        _delete(arg, sockfd);
    }
    if (strcmp(command, "add") == 0) {
        _add(arg, sockfd);
    }
    if (strcmp(command, "remove") == 0) {
        _remove(arg, sockfd);
    }

    //Close socket descriptor
    close(sockfd);

    return 0;
}


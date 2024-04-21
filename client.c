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
    char username[10], filename[10], tempFilename[20];
    strcpy(tempFilename, userFilename);
    char *token = strtok(tempFilename, "/");
    if (token != NULL) {
        strcpy(username, token);
        token = strtok(NULL, "/");
        if (token != NULL) {
            strcpy(filename, token);
            //printf("%s and %s\n", username, filename);
        } else {
            printf("Error: Missing filename\n");
            return;
        }
    }

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
    printf("Finished uploading file %s\n", filename);
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

    
    char username[10], filename[10], tempFilename[20];
    strcpy(tempFilename, userFilename);
    char *token = strtok(tempFilename, "/");
    if (token != NULL) {
        strcpy(username, token);
        token = strtok(NULL, "/");
        if (token != NULL) {
            strcpy(filename, token);
            //printf("%s and %s\n", username, filename);
        } else {
            printf("Error: Missing filename\n");
            return;
        }
    }

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
    char username[10], filename[10], tempFilename[20];
    strcpy(tempFilename, userFilename);
    char *token = strtok(tempFilename, "/");
    if (token != NULL) {
        strcpy(username, token);
        token = strtok(NULL, "/");
        if (token != NULL) {
            strcpy(filename, token);
            //printf("%s and %s\n", username, filename);
        } else {
            printf("Error: Missing filename\n");
            return;
        }
    }

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
    struct sockaddr_in serv_addr;

    if (argc != 3){
		printf ("Usage: %s <ip of server> <port #>\n",argv[0]);
		exit(0);
    }
    //Declare socket file descriptor and buffer
    int sockfd;
    char input[30], command[10], arg[20];

    // Create socket -- IPv4 and TCP
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf ("Error: Could not create socket! \n");
        return 1;
    }
    
    serv_addr.sin_family = AF_INET;  // Specify IPv4 address
    serv_addr.sin_port = htons (atoi (argv[2])); // host to network address short
    
    // Convert IP address from text format to binary
    if (inet_pton (AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
    {
        printf ("Could not convert IP address from text format to binary!\n");
        return 1;
    }
    
    // Connect to server
    if (connect (sockfd, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0)
    {
        printf ("Error: Connect failed! \n");
        return 1;
    }

    // Prompt user to enter command
    printf("Please enter a command: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;

    write(sockfd, input, strlen(input) + 1);

    char *token = strtok(input, " ");
    strcpy(command, token);
    token = strtok(NULL, " ");
    strcpy(arg, token);

    if (strcmp(command, "upload") == 0) {
        _upload(arg, sockfd);
    }
    else if (strcmp(command, "download") == 0) {
        _download(arg, sockfd);
    }
    else if (strcmp(command, "list") == 0) {
        _list(arg, sockfd);
    }
    else if (strcmp(command, "delete") == 0) {
        _delete(arg, sockfd);
    }
    else if (strcmp(command, "add") == 0) {
        _add(arg, sockfd);
    }
    else if (strcmp(command, "remove") == 0) {
        _remove(arg, sockfd);
    }
    else {
        printf("Invalid command!\n");
    }

    //Close socket descriptor
    close(sockfd);

    return 0;
}


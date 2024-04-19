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

}

void _download(char* userFilename, int socket) {

}

void _list(char* userFilename, int socket) {

}

void _delete(char* userFilename, int socket) {

}

void _add(char* userFilename, int socket) {

}

void _remove(char* userFilename, int socket) {

}

int main(int argc, char *argv[]){
    //Get server IP and port number from command line
    if (argc != 3){
		printf ("Usage: %s <ip of server> <port #>\n",argv[0]);
		exit(0);
    }
    //Declare socket file descriptor and buffer
    int sockfd;
    char *input, *command, *param;

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

    while (1) {

        //Connect to the server
        if (connect(sockfd, (struct sockaddr *)&servAddr, sizeof(struct sockaddr)) < 0){
            perror("Failure to connect to the server");
            exit(1);
        }

        // Prompt user to enter command
        printf("Please enter a command: ");
        scanf("%s", input);

        write(sockfd, input, sizeof(input));

        char *token = strtok(input, " ");
        strcpy(command, token);
        token = strtok(input, " ");
        strcpy(param, token);

        if (strcmp(command, "upload") == 0) {
            _upload(param, sockfd);
        }
        if (strcmp(command, "download") == 0) {
            _download(param, sockfd);
        }
        if (strcmp(command, "list") == 0) {
            _list(param, sockfd);
        }
        if (strcmp(command, "delete") == 0) {
            _delete(param, sockfd);
        }
        if (strcmp(command, "add") == 0) {
            _add(param, sockfd);
        }
        if (strcmp(command, "remove") == 0) {
            _remove(param, sockfd);
        }

    }

    //Close socket descriptor
    close(sockfd);

    return 0;
}


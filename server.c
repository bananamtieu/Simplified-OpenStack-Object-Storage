/*
Chan Nam Tieu
01/20/2022
Lab 3: TCP/IP Socket Programming
Step 1: TCP server that accepts a client connection for file transfer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

//Declare socket file descriptor.
int sockfd, connfd, rb, sin_size;

//Declare receiving and sending buffers of size 10 bytes
char rbuf[10], sbuf[10];

//Declare server address to which to bind for receiving messages and client address to fill in sending address
struct sockaddr_in servAddr, clienAddr;


int main(int argc, char *argv[]){
    //Get from the command line, server IP, src and dst files.
    if (argc != 3){
        printf ("Usage: %s <# partition power> <client's IP> \n",argv[0]);
        exit(0);
    }

    int partition = atoi(argv[1]);


    //Open a TCP socket, if successful, returns a descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("cannot create socket");
        exit(0);
    }

    int portNumber;
    printf("Please enter a port number: ");
    scanf("%d", &portNumber);

    //Setup the server address to bind using socket addressing structure
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(portNumber);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind IP address and port for server endpoint socket
    if ((bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr))) < 0){
        perror("Failure to bind server address to the endpoint socket");
        exit(0);
    }

    // Server listening to the socket endpoint, and can queue 5 client requests
    printf("Server listening/waiting for client at port %d\n", portNumber);
    listen(sockfd, 5);


    //Server accepts the connection and call the connection handler
    sin_size = sizeof(clienAddr);
    if ((connfd = accept(sockfd, (struct sockaddr *)&clienAddr, (socklen_t *)&sin_size)) < 0){
        perror("Failure to accept connection to the client");
        exit(0);
    }
    while(1) {

    }

    //close socket descriptor
    close(sockfd);

    return 0;
}

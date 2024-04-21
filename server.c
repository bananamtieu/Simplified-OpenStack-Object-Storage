/*
Chan Nam Tieu
01/20/2022
Lab 3: TCP/IP Socket Programming
Step 1: TCP server that accepts a client connection for file transfer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <openssl/evp.h>

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

int numDisks = 0;
struct disk DiskList[MAX_NUMDISKS];

//Declare socket file descriptor.
int sockfd, connfd, rb, sin_size;

//Declare receiving and sending buffers of size 10 bytes
char rbuf[10], sbuf[10];

//Declare server address to which to bind for receiving messages and client address to fill in sending address
struct sockaddr_in servAddr, clienAddr;

int getDiskIndex(char *diskIp) {
    for (int i = 0; i < numDisks; i++) {
        if (strcmp(DiskList[i].diskIp, diskIp) == 0) {
            return i;
        }
    }
    return -1;
}

void md5_hash(const char *input, unsigned char *output) {
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned int md_len;

    OpenSSL_add_all_digests();
    md = EVP_get_digestbyname("md5");

    mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, input, strlen(input));
    EVP_DigestFinal_ex(mdctx, output, &md_len);
    EVP_MD_CTX_free(mdctx);
}

void hex_to_binary(const char *hex_string, char *binary_string) {
    int i, j;
    for (i = 0, j = 0; hex_string[i] != '\0'; i++, j += 4) {
        switch (hex_string[i]) {
            case '0':
                strcat(binary_string + j, "0000");
                break;
            case '1':
                strcat(binary_string + j, "0001");
                break;
            case '2':
                strcat(binary_string + j, "0010");
                break;
            case '3':
                strcat(binary_string + j, "0011");
                break;
            case '4':
                strcat(binary_string + j, "0100");
                break;
            case '5':
                strcat(binary_string + j, "0101");
                break;
            case '6':
                strcat(binary_string + j, "0110");
                break;
            case '7':
                strcat(binary_string + j, "0111");
                break;
            case '8':
                strcat(binary_string + j, "1000");
                break;
            case '9':
                strcat(binary_string + j, "1001");
                break;
            case 'A':
            case 'a':
                strcat(binary_string + j, "1010");
                break;
            case 'B':
            case 'b':
                strcat(binary_string + j, "1011");
                break;
            case 'C':
            case 'c':
                strcat(binary_string + j, "1100");
                break;
            case 'D':
            case 'd':
                strcat(binary_string + j, "1101");
                break;
            case 'E':
            case 'e':
                strcat(binary_string + j, "1110");
                break;
            case 'F':
            case 'f':
                strcat(binary_string + j, "1111");
                break;
            default:
                printf("Invalid hexadecimal digit '%c'\n", hex_string[i]);
                return;
        }
    }
}
int main(int argc, char *argv[]){
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


    /*
    const char *input = "ctieu/server.c";
    
    unsigned char output[EVP_MAX_MD_SIZE];

    md5_hash(input, output);

    printf("MD5 Hash of '%s' is: ", input);
    size_t mdSize = EVP_MD_size(EVP_md5());

    char hexRep[mdSize*2], binRep[mdSize*8];
    for (int i = 0; i < mdSize; i++) {
        sprintf(hexRep + 2*i, "%02x", output[i]);
    }
    hex_to_binary(hexRep, binRep);
    printf("%s\n", binRep);
    */


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
            perror("Failure to bind server address to the endpoint socket. Finding the next available port.");
        }
        else {
            bindFail = 0;
        }
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

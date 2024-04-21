#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <set>
#include <vector>
#include <map>
#include "preprocess.h"

#define MAX_LOGIN_NAME 20
#define MAX_IPLENGTH 20
#define MAX_NUMFILES 100
#define MAX_PATHLENGTH 40

using namespace std;

struct Disk {
    char diskIp[MAX_IPLENGTH];
    int numFiles;
    vector<string> fileList;
};

vector<Disk> DiskList;

void _upload(const char* userFilename, int socket, int partition, char *loginName,
    vector<int> partitionArray, vector<string> DPAHelper, set<string> userFileHashSet) {
    char username[10], filename[10], tempFilename[20];
    strcpy(tempFilename, userFilename);
    stringstream ss(tempFilename);
    string token;

    if (getline(ss, token, '/')) {
        strcpy(username, token.c_str());
        if (getline(ss, token, '/')) {
            strcpy(filename, token.c_str());
        } else {
            cout << "Error: Missing filename" << endl;
            return;
        }
    }

    const char* directory = "/tmp/tempFile/";
    struct stat sb;
    if (stat(directory, &sb) != 0) {
        cout << "Temporary directory does not exist. Creating " << directory << endl;
        char command[100];
        strcpy(command, "mkdir -p /tmp/tempFile/");
        if (system(command) != 0) {
            cout << "Error: Failed to create directory." << endl;
            return;
        }
    } else {
        cout << "Temporary directory exists." << endl;
    }

    char filePath[50];
    strcpy(filePath, directory);
    strcat(filePath, filename);

    ofstream fileToUpload(filePath, ios::binary);
    char buff[1024];
    int bytesRead;
    cout << "File directory is " << filePath << endl;
    while ((bytesRead = read(socket, buff, 1024)) > 0) {
        fileToUpload.write(buff, bytesRead);
    }
    fileToUpload.close();
    cout << "Finished uploading file to server." << endl;

    string userFileHash = md5_hash(userFilename, partition);
    cout << "Hashed value is " << userFileHash << endl;

}

int main(int argc, char *argv[]) {
    // Declare socket file descriptor.
    int sockfd, connfd;

    // Declare server address to which to bind for receiving messages and client address to fill in sending address
    struct sockaddr_in servAddr, clienAddr;

    // Get from the command line, server IP, src and dst files.
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <# partition power> <available disks> " << endl;
        exit(0);
    }

    int partition = atoi(argv[1]);
    int numDisks = argc - 2;

    for (int i = 0; i < numDisks; i++) {
        Disk newDisk;
        strcpy(newDisk.diskIp, argv[i + 2]);
        DiskList.push_back(newDisk);
    }

    char login_name[MAX_LOGIN_NAME];
    getlogin_r(login_name, MAX_LOGIN_NAME);

    map<string, int> diskIndex;
    for (size_t i = 0; i < DiskList.size(); ++i) {
        diskIndex[DiskList[i].diskIp] = i;
    }

    int numPartition = (int) pow(2, partition);
    vector<int> partitionArray(numPartition);
    vector<string> DPAHelper(numPartition);

    for (int pNum = 0; pNum < numPartition; pNum++) {
        for (int i = 0; i < numDisks; i++) {
            if (pNum >= numPartition * i / numDisks && pNum < numPartition * (i + 1) / numDisks) {
                cout << pNum << ",";
                partitionArray[pNum] = i;
            }
        }
    }

    set<string> userFileHashSet;

    // Open a TCP socket, if successful, returns a descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("cannot create socket");
        exit(0);
    }

    int bindFail = 1;
    int portNumber;
    srand(time(0));
    while (bindFail) {
        portNumber = rand() % 8400 + 1024;
        // Setup the server address to bind using socket addressing structure
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(portNumber);
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        // Bind IP address and port for server endpoint socket
        if ((bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr))) < 0) {
            perror("Failure to bind server address to the endpoint socket");
            exit(0);
        } else {
            bindFail = 0;
        }
    }

    // Server listening to the socket endpoint, and can queue 5 client requests
    cout << "Server listening/waiting for client at port " << portNumber << endl;
    listen(sockfd, 5);

    // Server accepts the connection and call the connection handler
    size_t sin_size = sizeof(clienAddr);
    char buff[1024];
    char cmdInput[30], command[10], arg[20];
    while (true) {
        if ((connfd = accept(sockfd, (struct sockaddr *)&clienAddr, (socklen_t *)&sin_size)) < 0) {
            perror("Failure to accept connection to the client");
            exit(0);
        }
        cout << "Connection Established with client IP: " << inet_ntoa(clienAddr.sin_addr) << " and Port: " << ntohs(clienAddr.sin_port) << endl;
        ssize_t bytes_read = read(connfd, buff, sizeof(buff) - 1); // -1 to leave space for null terminator
        if (bytes_read < 0) {
            perror("Error reading from socket");
            exit(1);
        }
        // Ensure cmdInput is properly null-terminated
        buff[bytes_read] = '\0'; // Null-terminate the buffer
        strcpy(cmdInput, buff); // Copy buffer to cmdInput

        stringstream ss(cmdInput);
        string token;
        if (getline(ss, token, ' ')) {
            strcpy(command, token.c_str());
            if (getline(ss, token, ' ')) {
                strcpy(arg, token.c_str());
            }
        }
        cout << "userFilename = " << arg << endl; // Ensure newline to flush output


        if (strcmp(command, "upload") == 0) {
            _upload(arg, connfd, partition, login_name, partitionArray, DPAHelper, userFileHashSet);
        } else if (strcmp(command, "download") == 0) {
            //_download(arg, sockfd);
        } else if (strcmp(command, "list") == 0) {
            //_list(arg, sockfd);
        } else if (strcmp(command, "delete") == 0) {
            //_delete(arg, sockfd);
        } else if (strcmp(command, "add") == 0) {
            //_add(arg, sockfd);
        } else if (strcmp(command, "remove") == 0) {
            //_remove(arg, sockfd);
        } else {
            cout << "Invalid command!" << endl;
        }
    }

    // Close socket descriptor
    close(sockfd);

    return 0;
}

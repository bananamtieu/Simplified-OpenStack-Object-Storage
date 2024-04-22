#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

void _upload(const string& userFilename, int socket) {
    string username, filename;
    stringstream tempFilename(userFilename);
    getline(tempFilename, username, '/');
    getline(tempFilename, filename, '/');

    char buff[1024];
    ifstream fileToUpload(filename, ios::binary);
    if (fileToUpload.is_open()) {
        while (fileToUpload.read(buff, sizeof(buff))) {
            write(socket, buff, sizeof(buff)); // send 2
        }
        write(socket, "EOF", 4);
        fileToUpload.close();
    }
    else {
        cout << "File does not exist: " << filename << endl;
        return;
    }

    cout << "Finished uploading file " << filename << endl;
    read(socket, buff, sizeof(buff));

    cout << "Server's message: " << buff << endl;
}

void _download(const string& userFilename, int socket) {
    char buff[1024];
    read(socket, buff, sizeof(buff));
    if (strcmp(buff, "The file is not exist.") == 0) {
        cout << buff << endl;
    }

    string username, filename;
    stringstream tempFilename(userFilename);
    getline(tempFilename, username, '/');
    getline(tempFilename, filename, '/');
    
    ofstream fileToDownload(filename, ios::binary);
    if (filename != "q") {
        int bytesRead;
        while ((bytesRead = read(socket, buff, sizeof(buff))) > 0) {
            if (bytesRead == 4 && strcmp(buff, "EOF") == 0) {
                break; // End of file transmission
            }
            fileToDownload.write(buff, bytesRead);
            cout << buff;
        }
        fileToDownload.close();
    }

    ifstream downloadedFile(filename, ios::binary);
    if (filename != "q") {
        if (downloadedFile.is_open()) {
            while (downloadedFile.read(buff, sizeof(buff))) {
                cout << buff;
            }
        }
        cout << endl;
        downloadedFile.close();
    }
}

void _list(const string& userFilename, int socket) {
    string username, filename;
    stringstream tempFilename(userFilename);
    getline(tempFilename, username, '/');
    getline(tempFilename, filename, '/');
    
    ofstream outputFile("output.txt", ios::binary);
    char buff[1024];
    int bytesRead;
    while ((bytesRead = read(socket, buff, sizeof(buff))) > 0) {
        outputFile.write(buff, bytesRead);
    }
    outputFile.close();
}

void _delete(const string& userFilename, int socket) {
    char buff[1024];
    read(socket, buff, sizeof(buff));
    if (strcmp(buff, "The file is not exist.") == 0) {
        cout << buff << endl;
    }

    read(socket, buff, sizeof(buff));
    cout << buff << endl;
}

void _add(const string& userFilename, int socket) {
    char buff[1024];
    read(socket, buff, sizeof(buff));
    cout << "Server's message: " << buff << endl;
}

void _remove(const string& userFilename, int socket) {
    char buff[1024];
    read(socket, buff, sizeof(buff));
    cout << "Server's message: " << buff << endl;
}

int main(int argc, char *argv[]){
    if (argc != 3){
        cerr << "Usage: " << argv[0] << " <ip of server> <port #>" << endl;
        return 1;
    }

    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "Error: Could not create socket!" << endl;
        return 1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        cerr << "Could not convert IP address from text format to binary!" << endl;
        return 1;
    }
    
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Error: Connect failed!" << endl;
        return 1;
    }

    string input, command, arg;
    cout << "Please enter a command: ";
    getline(cin, input);

    write(sockfd, input.c_str(), input.size() + 1); // send 1

    stringstream ss(input);
    getline(ss, command, ' ');
    getline(ss, arg, ' ');

    if (command == "upload") {
        _upload(arg, sockfd);
    }
    else if (command == "download") {
        _download(arg, sockfd);
    }
    else if (command == "list") {
        _list(arg, sockfd);
    }
    else if (command == "delete") {
        _delete(arg, sockfd);
    }
    else if (command == "add") {
        _add(arg, sockfd);
    }
    else if (command == "remove") {
        _remove(arg, sockfd);
    }
    else {
        printf("Invalid command!\n");
    }

    close(sockfd);

    return 0;
}

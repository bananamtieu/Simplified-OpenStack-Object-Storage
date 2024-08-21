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
#define BUFF_SIZE 1024

void _upload(const string& userFilename, int socket) {
    string username, filename;
    stringstream tempFilename(userFilename);
    getline(tempFilename, username, '/');
    getline(tempFilename, filename, '/');

    char buff[BUFF_SIZE] = {0};
    ifstream fileToUpload(filename, ios::binary);

    if (!fileToUpload) {
        cerr << "Error opening file" << endl;
        return;
    }

    while (fileToUpload.read(buff, BUFF_SIZE) || fileToUpload.gcount() > 0) {
        send(socket, buff, fileToUpload.gcount(), 0);
    }
    // Signal that no more data will be sent
    shutdown(socket, SHUT_WR);

    cout << "Finished uploading file " << filename << endl;
    fileToUpload.close();

    recv(socket, buff, BUFF_SIZE, 0);
    cout << "Server's message: " << buff << endl;
}

void _download(const string& userFilename, int socket) {
    char buff[BUFF_SIZE];
    read(socket, buff, BUFF_SIZE);
    if (strcmp(buff, "File not found!") == 0) {
        cout << buff << endl;
        return;
    }

    string username, filename;
    stringstream tempFilename(userFilename);
    getline(tempFilename, username, '/');
    getline(tempFilename, filename, '/');
    
    ofstream fileToDownload("OpenStack_" + filename, ios::binary);
    if (filename != "q") {
        int bytesRead;
        while ((bytesRead = read(socket, buff, BUFF_SIZE)) > 0) {
            fileToDownload.write(buff, bytesRead);
        }
        fileToDownload.close();
    }

    cout << userFilename << " was downloaded and saved to OpenStack_" << filename << endl;
}

void _list(const string& userFilename, int socket) {
    string username, filename;
    stringstream tempFilename(userFilename);
    getline(tempFilename, username, '/');
    getline(tempFilename, filename, '/');
    
    char buff[BUFF_SIZE];
    while (read(socket, buff, BUFF_SIZE) > 0) {
        cout << buff;
    }
}

void _delete(const string& userFilename, int socket) {
    char buff[BUFF_SIZE];
    read(socket, buff, BUFF_SIZE);
    if (strcmp(buff, "File not found!") == 0) {
        cout << buff << endl;
        return;
    }

    read(socket, buff, BUFF_SIZE);
    cout << buff << endl;
}

void _add(const string& userFilename, int socket) {
    char buff[BUFF_SIZE];
    read(socket, buff, BUFF_SIZE);
    cout << "Server's message: " << buff << endl;
}

void _remove(const string& userFilename, int socket) {
    char buff[BUFF_SIZE];
    read(socket, buff, BUFF_SIZE);
    cout << "Server's message: " << buff << endl;
}

void _clean(int socket) {
    char buff[BUFF_SIZE];
    read(socket, buff, BUFF_SIZE);
    cout << "Server's message: " << buff << endl;
}

int main(int argc, char const *argv[]){
    if (argc != 3){
        cerr << "Usage: " << argv[0] << " <ip of server> <port #>" << endl;
        return 1;
    }

    int sockfd = 0;
    struct sockaddr_in serv_addr;
    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Socket creation error\n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid address/Address not supported\n";
        return -1;
    }
    
    // Connecting to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection Failed\n";
        return -1;
    }

    string input, command, arg;
    cout << "Please enter a command: ";
    getline(cin, input);

    send(sockfd, input.c_str(), input.size() + 1, 0);

    stringstream ss(input);
    getline(ss, command, ' ');
    getline(ss, arg, ' ');

    if (command.compare("upload") == 0) {
        _upload(arg, sockfd);
    }
    else if (command.compare("download") == 0) {
        _download(arg, sockfd);
    }
    else if (command.compare("list") == 0) {
        _list(arg, sockfd);
    }
    else if (command.compare("delete") == 0) {
        _delete(arg, sockfd);
    }
    else if (command.compare("add") == 0) {
        _add(arg, sockfd);
    }
    else if (command.compare("remove") == 0) {
        _remove(arg, sockfd);
    }
    else if (command.compare("clean") == 0) {
        _clean(sockfd);
    }
    else {
        cout << "Invalid command!" << endl;
    }

    close(sockfd);

    return 0;
}

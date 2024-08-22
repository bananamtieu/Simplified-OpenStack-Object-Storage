#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <set>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>
#include <mutex>
#include <bits/stdc++.h>
#include "preprocess.h"
#include "storageHelper.h"

using namespace std;
#define BUFF_SIZE 1024
#define MAX_CLIENTS 5

mutex mtx;
vector<int> partitionArray;
vector<Disk> DiskList;
set<int> userFileHashSet; // hash values
vector<string> DPAHelper; // file name associated with that parition
map<string, int> diskIndex; // diskIp --> diskIndex
int client_count = 0;

void printDiskList() {
    cout << "Printing Disk List..." << endl;
    for (Disk _disk : DiskList) {
        cout << "Disk: " << _disk.diskIp << endl;
        cout << "  Files:" << endl;
        for (string filePath : _disk.fileList) {
            cout << "   " << filePath << endl;
        }
    }
}

void _upload(const string& userFilename, int socket, int partition, const string& login_name) {
    string username = userFilename.substr(0, userFilename.find("/"));
    string filename = userFilename.substr(userFilename.find("/") + 1);

    if (filename.length() == 0) {
        cout << "Error: Missing filename" << endl;
        return;
    }

    string directory = "/tmp/tempFile/";
    struct stat sb;
    if (stat(directory.c_str(), &sb) != 0) {
        cout << "Temporary directory does not exist. Creating " << directory << endl;
        string command = "mkdir -p " + directory;
        if (system(command.c_str()) != 0) {
            cout << "Error: Failed to create directory." << endl;
            return;
        }
    }

    string filePath = directory + filename;
    ofstream fileToUpload(filePath, ios::binary);
    char buff[BUFF_SIZE];
    
    if (!fileToUpload) {
        cerr << "Error creating file!" << endl;
        return;
    }

    int bytesRead;
    while ((bytesRead = read(socket, buff, BUFF_SIZE)) > 0) {
        fileToUpload.write(buff, bytesRead);
    }
    memset(buff, 0, BUFF_SIZE);
    cout << "Finished uploading file to server." << endl;
    fileToUpload.close();
    
    mtx.lock(); // Lock the mutex before modifying the shared resource
    string userFileHash = md5_hash(userFilename.c_str(), partition);
    int hashValue = stoi(userFileHash, nullptr, 2);
    
    int mainDisk = partitionArray[hashValue];
    int backupDisk = (mainDisk == DiskList.size() - 1) ? 0 : mainDisk + 1;

    userFileHashSet.insert(hashValue);
    DPAHelper[hashValue] = userFilename;

    ofstream commandfile("commandfile.sh");
    commandfile << "ssh -o StrictHostKeyChecking=no " << login_name << "@" << DiskList[mainDisk].diskIp 
                << " \"mkdir -p /tmp/" << login_name << "/" << username << "\"" << endl;
    commandfile << "scp -B " << filePath << " " << login_name << "@" << DiskList[mainDisk].diskIp 
                << ":/tmp/" << login_name << "/" << username << "/" << filename << endl;
    commandfile.close();
    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    // Create command file to run commands for backup disk
    commandfile.open("commandfile.sh");
    commandfile << "ssh -o StrictHostKeyChecking=no " << login_name << "@" << DiskList[backupDisk].diskIp 
                << " \"mkdir -p /tmp/" << login_name << "/backupfolder\"" << endl;
    commandfile << "ssh -o StrictHostKeyChecking=no " << login_name << "@" << DiskList[backupDisk].diskIp 
                << " \"mkdir -p /tmp/" << login_name << "/backupfolder/" << username << "\"" << endl;
    commandfile << "scp -B " << filePath << " " << login_name << "@" << DiskList[backupDisk].diskIp 
                << ":/tmp/" << login_name << "/backupfolder/" << username << "/" << filename << endl;
    commandfile.close();
    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    // Remove temporary file and directory
    remove(filePath.c_str());
    rmdir(directory.c_str());

    string mainDiskPath = login_name + "/" + username + "/" + filename;
    string backupDiskPath = login_name + "/backupFolder/" + username + "/" + filename;
    DiskList[mainDisk].fileList.push_back(mainDiskPath);
    DiskList[backupDisk].fileList.push_back(backupDiskPath);
    mtx.unlock(); // Unlock the mutex after modifying the shared resource

    string result = username + "/" + filename + " was mainly saved in " + DiskList[mainDisk].diskIp 
        + " and saved in " + DiskList[backupDisk].diskIp + " for backup.";
    send(socket, result.c_str(), BUFF_SIZE, 0);
}

void _download(const string& userFilename, int socket, int partition, const string& login_name) {
    string username = userFilename.substr(0, userFilename.find("/"));
    string filename = userFilename.substr(userFilename.find("/") + 1);

    if (filename.length() == 0) {
        cout << "Error: Missing filename" << endl;
        return;
    }

    char buff[BUFF_SIZE];
    auto dpaIt = find(DPAHelper.begin(), DPAHelper.end(), userFilename);
    if (dpaIt == DPAHelper.end()) {
        string message = "File not found!";
        write(socket, message.c_str(), message.size());
        return;
    } else {
        string message = "Start downloading...";
        write(socket, message.c_str(), message.size());
    }

    string userFileHash = md5_hash(userFilename.c_str(), partition);
    int hashValue = stoi(userFileHash, nullptr, 2);
    
    int mainDisk = partitionArray[hashValue];
    int backupDisk = (mainDisk == DiskList.size() - 1) ? 0 : mainDisk + 1;

    // RestoreFiles(login_name, username, filename, DiskList, backupDisk, mainDisk);

    string directory = "/tmp/tempFile/";
    struct stat sb;
    if (stat(directory.c_str(), &sb) != 0) {
        cout << "Temporary directory does not exist. Creating " << directory << endl;
        string command = "mkdir -p " + directory;
        if (system(command.c_str()) != 0) {
            cout << "Error: Failed to create directory." << endl;
            return;
        }
    }

    // Create a script commandfile.sh to run command: CopyFile
    ofstream commandfile("commandfile.sh");
    commandfile << "#!/bin/bash" << endl;
    commandfile << "scp -B " << login_name << "@" << DiskList[mainDisk].diskIp << ":/tmp/" << login_name << "/" << username << "/"
                << filename << " " << directory << filename << endl;
    commandfile.close();
    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    string filePath = directory + filename;

    ifstream fileToDownload(filePath, ios::binary);
    while (fileToDownload.read(buff, BUFF_SIZE) || fileToDownload.gcount() > 0) {
        send(socket, buff, fileToDownload.gcount(), 0);
    }

    // Signal that no more data will be sent
    shutdown(socket, SHUT_WR);
    fileToDownload.close();

    // Clean up
    remove(filePath.c_str());
    rmdir(directory.c_str());
    cout << username << "/" << filename << " was downloaded from " << DiskList[mainDisk].diskIp << endl;
}

void _list(const string& username, int socket, int partition, const string& login_name) {
    ofstream allFiles("allFiles.sh");
    if (!allFiles.is_open()) {
        cerr << "Error: Unable to open allFiles.sh" << endl;
        return;
    }
    allFiles.close();
    system("chmod 700 allFiles.sh");

    string directory = "/tmp/tempFile/";
    struct stat sb;
    if (stat(directory.c_str(), &sb) != 0) {
        cout << "Temporary directory does not exist. Creating " << directory << endl;
        string command = "mkdir -p " + directory;
        if (system(command.c_str()) != 0) {
            cout << "Error: Failed to create directory." << endl;
            return;
        }
    }

    for (int mainDisk = 0; mainDisk < DiskList.size(); mainDisk++) {
        string diskIp = DiskList[mainDisk].diskIp;
        const auto& filelist = DiskList[mainDisk].fileList;

        for (const string& LoginUserFile : filelist) {
            if (LoginUserFile.find(username) != string::npos && LoginUserFile.find("backupFolder") == string::npos) {
                string _filename = LoginUserFile.substr(LoginUserFile.find_last_of('/') + 1);
                string userFilename = LoginUserFile.substr(LoginUserFile.find_first_of('/') + 1);
                string userFileHash = md5_hash(userFilename.c_str(), partition);
                int hashValue = stoi(userFileHash, nullptr, 2);

                int backupDisk = (partitionArray[hashValue] == DiskList.size() - 1) ? 0 : partitionArray[hashValue] + 1;

                // RestoreFiles(login_name, username, filename, DiskList, backupDisk, mainDisk);

                string GetDiskList = "ssh -o StrictHostKeyChecking=no " + login_name + "@" + diskIp + " \"cd /tmp/" + login_name + "/" + username + " ; ls -lrt >> ~/output" + diskIp + ".txt\"";
                string CopyFile = "scp -B " + login_name + "@" + diskIp + ":~/output" + diskIp + ".txt " + directory + "output" + diskIp + ".txt";
                string RemoveOutput = "ssh -o StrictHostKeyChecking=no " + login_name + "@" + diskIp + " \"rm ~/output" + diskIp + ".txt\"";

                ofstream allFiles("allFiles.sh", ios_base::app);
                if (!allFiles.is_open()) {
                    cerr << "Error: Unable to open allFiles.sh for appending" << endl;
                    return;
                }
                allFiles << GetDiskList << endl;
                allFiles << CopyFile << endl;
                allFiles << RemoveOutput << endl;
                allFiles.close();
            }
        }
    }

    system("sh allFiles.sh");
    remove("allFiles.sh");

    string outputDirectory = username + "_fileList.txt";

    ofstream outfile(outputDirectory, ios::binary);
    if (!outfile.is_open()) {
        cerr << "Error: Unable to open output file for writing" << endl;
        return;
    }

    for (int mainDisk = 0; mainDisk < DiskList.size(); mainDisk++) {
        string diskIp = DiskList[mainDisk].diskIp;
        string entryPath = directory + "output" + diskIp + ".txt";

        ifstream infile(entryPath);
        if (infile.is_open()) {
            outfile << infile.rdbuf();
            infile.close();
            remove(entryPath.c_str());
        }
    }
    outfile.close();

    ifstream fileList(outputDirectory, ios::binary);
    char buff[BUFF_SIZE];
    while (fileList.read(buff, BUFF_SIZE) || fileList.gcount() > 0) {
        send(socket, buff, fileList.gcount(), 0);
    }
    // Signal that no more data will be sent
    shutdown(socket, SHUT_WR);
    fileList.close();

    remove(outputDirectory.c_str());
    rmdir(directory.c_str());
    cout << "Listing completed!" << endl;
}

void _delete(const string& userFilename, int socket, int partition, const string& login_name) {
    // Check if the file exists
    char buff[BUFF_SIZE];
    auto dpaIt = find(DPAHelper.begin(), DPAHelper.end(), userFilename);
    if (dpaIt == DPAHelper.end()) {
        string message = "File not found!";
        write(socket, message.c_str(), message.size());
        return;
    } else {
        string message = "Start deleting...";
        write(socket, message.c_str(), message.size());
    }

    // Split UserFileName into username and filename
    string username = userFilename.substr(0, userFilename.find("/"));
    string filename = userFilename.substr(userFilename.find("/") + 1);

    if (filename.length() == 0) {
        cout << "Error: Missing filename" << endl;
        return;
    }

    mtx.lock(); // Lock the mutex before modifying the shared resource
    string userFileHash = md5_hash(userFilename.c_str(), partition);
    int hashValue = stoi(userFileHash, nullptr, 2);
    
    int mainDisk = partitionArray[hashValue];
    int backupDisk = (mainDisk == DiskList.size() - 1) ? 0 : mainDisk + 1;

    // Delete file from main disk
    ofstream commandfile("commandfile.sh");
    if (!commandfile.is_open()) {
        cerr << "Error: Unable to open commandfile.sh" << endl;
        return;
    }
    commandfile << "ssh -o StrictHostKeyChecking=no " << login_name << "@" << DiskList[mainDisk].diskIp 
                << " \"cd /tmp/" << login_name << "/" << username << " ; rm " << filename << "\"" << endl;
    commandfile.close();
    system("sh commandfile.sh");
    remove("commandfile.sh");

    // Delete file from backup disk
    commandfile.open("commandfile.sh");
    if (!commandfile.is_open()) {
        cerr << "Error: Unable to open commandfile.sh" << endl;
        return;
    }
    commandfile << "ssh -o StrictHostKeyChecking=no " << login_name << "@" << DiskList[backupDisk].diskIp 
                << " \"cd /tmp/" << login_name << "/backupfolder/" << username << " ; rm " << filename << "\"" << endl;
    commandfile.close();
    system("sh commandfile.sh");
    remove("commandfile.sh");

    // Delete file record in DiskList
    for (auto& _mainDisk : DiskList) {
        auto& filelist = _mainDisk.fileList;
        filelist.erase(remove(filelist.begin(), filelist.end(), userFilename), filelist.end());
    }

    // Delete file record in UserFileHashSet
    userFileHashSet.erase(hashValue);

    // Delete file record in DPAHelper
    DPAHelper[hashValue] = "";
    mtx.unlock(); // Unlock the mutex after modifying the shared resource

    // Send result back to client
    string result = username + "/" + filename + " was deleted from main disk " + DiskList[mainDisk].diskIp 
        + " and backup disk " + DiskList[backupDisk].diskIp + ".";
    write(socket, result.c_str(), result.size());
}

void _add(const string& newDiskIp, int socket, int partition, const string& login_name) {
    mtx.lock(); // Lock the mutex before modifying the shared resource

    Disk newDisk;
    newDisk.diskIp = newDiskIp;
    DiskList.push_back(newDisk);

    int numDisks = DiskList.size();
    diskIndex[newDiskIp] = numDisks - 1;
    int numPartition = partitionArray.size();

    for (int i = 0; i < numDisks - 1; ++i) {
        for (int pNum = 0; pNum < numPartition; ++pNum) {
            if (partitionArray[pNum] == i && (pNum + 1) % numDisks != (numDisks - 1)) {
                if (userFileHashSet.count(pNum) && i == numDisks - 2) {
                    string userFile = DPAHelper[pNum];
                    string username = userFile.substr(0, userFile.find("/"));
                    string filename = userFile.substr(userFile.find("/") + 1);

                    int oldBackupDisk = 0;
                    int newBackupDisk = numDisks - 1;

                    if (newBackupDisk != oldBackupDisk) {
                        moveBackup(login_name, username, filename, DiskList, oldBackupDisk, newBackupDisk);
                        deleteOldBackup(login_name, username, filename, DiskList, oldBackupDisk);

                        string filePath = login_name + "/backupFolder/" + username + "/" + filename;
                        DiskList[newBackupDisk].fileList.push_back(filePath);

                        auto& filelist = DiskList[oldBackupDisk].fileList;
                        filelist.erase(remove_if(filelist.begin(), filelist.end(),
                            [&](const string& file) { 
                                return file.find("/backupfolder/" + username + "/" + filename) != string::npos; 
                            }), filelist.end());
                    }
                }
            } else if (partitionArray[pNum] == i) {
                partitionArray[pNum] = diskIndex[newDiskIp];
                if (userFileHashSet.count(pNum)) {
                    string userFile = DPAHelper[pNum];
                    string username = userFile.substr(0, userFile.find("/"));
                    string filename = userFile.substr(userFile.find("/") + 1);

                    int oldMainDisk = i;
                    int newMainDisk = diskIndex[newDiskIp];
                    int oldBackupDisk = (i == numDisks - 2) ? 0 : (i + 1);
                    int newBackupDisk = 0;

                    if (newMainDisk != oldMainDisk) {
                        moveMain(login_name, username, filename, DiskList, oldMainDisk, newMainDisk);
                        deleteOldMain(login_name, username, filename, DiskList, oldMainDisk);

                        string filePath = login_name + "/" + username + "/" + filename;
                        DiskList[newMainDisk].fileList.push_back(filePath);

                        auto& filelist = DiskList[oldMainDisk].fileList;
                        filelist.erase(remove_if(filelist.begin(), filelist.end(),
                            [&](const string& file) { 
                                return file.find("/backupFolder/") == string::npos &&
                                    file.find(username + "/" + filename) != string::npos; 
                            }), filelist.end());
                    }

                    if (newBackupDisk != oldBackupDisk) {
                        moveBackup(login_name, username, filename, DiskList, oldBackupDisk, newBackupDisk);
                        deleteOldBackup(login_name, username, filename, DiskList, oldBackupDisk);

                        string filePath = login_name + "/backupFolder/" + username + "/" + filename;
                        DiskList[newBackupDisk].fileList.push_back(filePath);

                        auto& backupFileList = DiskList[oldBackupDisk].fileList;
                        backupFileList.erase(remove_if(backupFileList.begin(), backupFileList.end(),
                            [&](const string& file) { 
                                return file.find("/backupfolder/" + username + "/" + filename) != string::npos; 
                            }), backupFileList.end());
                    }
                }
            }
        }
    }
    mtx.unlock(); // Unlock the mutex after modifying the shared resource
    
    printDiskList();

    // Send result back to client
    string result = "All files are now in disks";
    for (const auto& disk : DiskList) {
        result += " " + disk.diskIp;
    }
    result += ".";
    write(socket, result.c_str(), result.size());
}

void _remove(const string& oldDiskIp, int socket, int partition, const string& login_name) {
    vector<int> oldPartitionArray(partitionArray);

    // Check if OldDisk exists
    if (diskIndex.find(oldDiskIp) == diskIndex.end()) {
        cout << "Invalid disk IP!" << endl;
        return;
    }
    int numDisks = DiskList.size();
    int numPartition = partitionArray.size();
    int oldDiskIndex = diskIndex[oldDiskIp];

    // Swap old disk and last disk
    if (oldDiskIndex != numDisks - 1) {
        Disk lastDisk = DiskList[numDisks - 1];
        DiskList[numDisks - 1] = DiskList[oldDiskIndex];
        DiskList[oldDiskIndex] = lastDisk;
        diskIndex[lastDisk.diskIp] = oldDiskIndex;
        diskIndex[oldDiskIp] = numDisks - 1;

        replace(partitionArray.begin(), partitionArray.end(), numDisks - 1, -1);
        replace(partitionArray.begin(), partitionArray.end(), oldDiskIndex, numDisks - 1);
        replace(partitionArray.begin(), partitionArray.end(), -1, oldDiskIndex);
        oldDiskIndex = numDisks - 1;
    }

    // Remove old disk record in partitionArray
    int count = 0;
    for (int pNum = 0; pNum < numPartition; ++pNum) {
        if (partitionArray[pNum] == oldDiskIndex) {
            partitionArray[pNum] = count%(numDisks - 1);
        }
        count++;
    }

    for (int pNum = 0; pNum < numPartition; ++pNum) {
        cout << partitionArray[pNum] << "  ";
    }
    cout << endl;

    for (int h = 0; h < numPartition; ++h) {
        if (oldPartitionArray[h] == oldDiskIndex - 1 && userFileHashSet.count(h)) {
            string userFile = DPAHelper[h];
            string username = userFile.substr(0, userFile.find("/"));
            string filename = userFile.substr(userFile.find("/") + 1);

            string filePath = login_name + "/backupFolder/" + username + "/" + filename;
            DiskList[oldDiskIndex + 1].fileList.push_back(filePath);
        }

        if (oldPartitionArray[h] == oldDiskIndex && userFileHashSet.count(h)) {
            string userFile = DPAHelper[h];
            string username = userFile.substr(0, userFile.find("/"));
            string filename = userFile.substr(userFile.find("/") + 1);

            int oldMainDisk = oldDiskIndex;
            int newMainDisk = partitionArray[h];
            int oldBackupDisk = (partitionArray[h] == (numDisks - 1)) ? 0 : oldDiskIndex + 1;
            int newBackupDisk = (partitionArray[h] == (oldDiskIndex - 1)) ? partitionArray[h] + 2 : partitionArray[h] + 1;

            if (DiskList[newMainDisk].diskIp != DiskList[oldMainDisk].diskIp) {
                string filePath = login_name + "/" + username + "/" + filename;
                DiskList[newMainDisk].fileList.push_back(filePath);
            }
            if (DiskList[newBackupDisk].diskIp != DiskList[oldBackupDisk].diskIp) {
                string filePath = login_name + "/backupFolder/" + username + "/" + filename;
                DiskList[newBackupDisk].fileList.push_back(filePath);
            }
        }
    }

    DiskList.erase(DiskList.begin() + oldDiskIndex);

    // Send result back to client
    string result = "All files are now in disks";
    for (const auto& disk : DiskList) {
        result += " " + disk.diskIp;
    }
    result += ".";
    write(socket, result.c_str(), result.size());
}

void _clean(int socket, int partition, const string& login_name) {
    mtx.lock(); // Lock the mutex before modifying the shared resource

    partitionArray.clear();
    for (auto& disk : DiskList) {
        disk.fileList.clear();
        string clearCommand = "ssh -o StrictHostKeyChecking=no " + login_name + "@" + disk.diskIp 
            + " \"rm -rf /tmp/" + login_name + "\"";
        system(clearCommand.c_str());
    }

    for (int hashSet : userFileHashSet) {
        DPAHelper[hashSet] = "";
    }

    userFileHashSet.clear();
    diskIndex.clear();

    mtx.unlock(); // Unlock the mutex after modifying the shared resource

    string message = "All disks have been cleared.";
    write(socket, message.c_str(), message.size());
}

void handleClient(int client_socket, int partition, const char *login_name) {
    char buff[BUFF_SIZE] = {0};

    ssize_t bytes_read = read(client_socket, buff, BUFF_SIZE);
    if (bytes_read < 0) {
        perror("Error reading from socket");
        exit(1);
    }

    string cmdInput = string(buff);
    cout << "Client's command: " << cmdInput << endl;

    string command = cmdInput.substr(0, cmdInput.find(" "));
    string arg = cmdInput.substr(cmdInput.find(" ") + 1);

    if (command == "upload") {
        _upload(arg, client_socket, partition, login_name);
    } else if (command == "download") {
        _download(arg, client_socket, partition, login_name);
    } else if (command == "list") {
        _list(arg, client_socket, partition, login_name);
    } else if (command == "delete") {
        _delete(arg, client_socket, partition, login_name);
    } else if (command == "add") {
        _add(arg, client_socket, partition, login_name);
    } else if (command == "remove") {
        _remove(arg, client_socket, partition, login_name);
    } else if (command == "clean") {
        _clean(client_socket, partition, login_name);
    } else {
        cout << "Invalid command!" << endl;
    }
    close(client_socket);
    client_count--;
}

int main(int argc, char *argv[]) {
    // Declare socket file descriptor.
    int server_fd, connfd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    vector<thread> threads;

    // Get from the command line, server IP, src and dst files.
    if (argc <= 2) {
        cerr << "Usage: " << argv[0] << " <# partition power> <available disks> " << endl;
        exit(0);
    }

    int partition = atoi(argv[1]);
    int numDisks = argc - 2;

    for (int i = 0; i < numDisks; ++i) {
        Disk newDisk;
        newDisk.diskIp = argv[i + 2];
        DiskList.push_back(newDisk);
        diskIndex[newDisk.diskIp] = i;
    }

    char login_name[MAX_LOGIN_NAME];
    getlogin_r(login_name, MAX_LOGIN_NAME);

    int numPartition = (2 << partition);
    partitionArray = vector<int>(numPartition);
    DPAHelper = vector<string>(numPartition);

    for (int pNum = 0; pNum < numPartition; ++pNum) {
        for (int i = 0; i < numDisks; i++) {
            if (pNum >= numPartition*i/numDisks && pNum < numPartition*(i + 1)/numDisks) {
                partitionArray[pNum] = i;
            }
        }
    }
    for (int pNum = 0; pNum < numPartition; ++pNum) {
        cout << partitionArray[pNum] << "  ";
    }
    cout << endl;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int bindFail = 1;
    int portNumber;
    srand(time(0));
    while (bindFail) {
        portNumber = rand() % 8400 + 1024;
        // Setting up the server address structure
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(portNumber);

        // Binding the socket to the specified IP and port
        if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0) {
            perror("Bind failed");
            exit(EXIT_FAILURE);
        } else {
            bindFail = 0;
        }
    }

    // Listening for incoming connections
    cout << "Server listening/waiting for client at port " << portNumber << endl;
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    while (true) { 
        // Accepting a client connection
        if ((connfd = accept(server_fd, (struct sockaddr *)&address, 
                        (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        if (client_count < MAX_CLIENTS) {
            client_count++;
            threads.emplace_back([connfd, partition, login_name]() {
                handleClient(connfd, partition, login_name);
            });
            cout << "Connection Established with client IP: " << inet_ntoa(address.sin_addr)
                << " and Port: " << ntohs(address.sin_port) << endl;
        } else {
            cerr << "Max client limit reached. Connection rejected.\n";
            close(connfd);
        }
        
    }
    // Close socket descriptor
    close(server_fd);

    return 0;
}

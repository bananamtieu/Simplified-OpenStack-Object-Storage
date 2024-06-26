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
#include <bits/stdc++.h>
#include "preprocess.h"
#include "storageHelper.h"

using namespace std;

void printDiskList(vector<Disk> DiskList) {
    cout << "Printing Disk List..." << endl;
    for (Disk _disk : DiskList) {
        cout << "Disk: " << _disk.diskIp << endl;
        cout << "  Files:" << endl;
        for (string filePath : _disk.fileList) {
            cout << filePath << endl;
        }
    }
}

void _upload(const char* userFilename, int socket, int partition, char *login_name, vector<int> &partitionArray, vector<Disk> &DiskList, vector<string> &DPAHelper, set<int> &userFileHashSet) {
    char username[10], filename[20], tempFilename[30];
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
    }

    char filePath[MAX_PATHLENGTH];
    strcpy(filePath, directory);
    strcat(filePath, filename);

    ofstream fileToUpload(filePath, ios::binary);
    char buff[1024];
    int bytesRead;
    
    while ((bytesRead = read(socket, buff, 1024)) > 0) { // recv 3
        if (bytesRead == 4 && strcmp(buff, "EOF") == 0) {
            break; // End of file transmission
        }
        fileToUpload.write(buff, bytesRead);
    }
    fileToUpload.close();
    cout << "Finished uploading file to server." << endl;

    string userFileHash = md5_hash(userFilename, partition);
    int hashValue = stoi(userFileHash, nullptr, 2);
    
    int mainDisk = partitionArray[hashValue];
    int backupDisk = ((partitionArray[hashValue] == DiskList.size() - 1)? 0 : partitionArray[hashValue] + 1);

    userFileHashSet.insert(hashValue);
    DPAHelper[hashValue] = userFilename;


    ofstream commandfile("commandfile.sh");
    commandfile << "ssh -o StrictHostKeyChecking=no " << login_name << "@" << DiskList[mainDisk].diskIp << " \"mkdir -p /tmp/" << login_name << "/" << username << "\"" << endl;
    commandfile << "scp -B " << directory << filename << " " << login_name << "@" << DiskList[mainDisk].diskIp << ":/tmp/" << login_name << "/" << username << "/" << filename << endl;
    commandfile.close();
    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    // Create command file to run commands for backup disk
    commandfile.open("commandfile.sh");
    commandfile << "ssh -o StrictHostKeyChecking=no " << login_name << "@" << DiskList[backupDisk].diskIp << " \"mkdir -p /tmp/" << login_name << "/backupfolder\"" << endl;
    commandfile << "ssh -o StrictHostKeyChecking=no " << login_name << "@" << DiskList[backupDisk].diskIp << " \"mkdir -p /tmp/" << login_name << "/backupfolder/" << username << "\"" << endl;
    commandfile << "scp -B " << directory << filename << " " << login_name << "@" << DiskList[backupDisk].diskIp << ":/tmp/" << login_name << "/backupfolder/" << username << "/" << filename << endl;
    commandfile.close();
    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    // Remove temporary file and directory
    remove(filePath);
    rmdir(directory);

    char mainDiskPath[MAX_PATHLENGTH], backupDiskPath[MAX_PATHLENGTH];
    sprintf(mainDiskPath, "%s/%s/%s", login_name, username, filename);
    sprintf(backupDiskPath, "%s/backupFolder/%s/%s", login_name, username, filename);
    DiskList[mainDisk].fileList.push_back(mainDiskPath);
    DiskList[backupDisk].fileList.push_back(backupDiskPath);

    sprintf(buff, "%s/%s was mainly saved in %s and saved in %s for backup.", username, filename, DiskList[mainDisk].diskIp, DiskList[backupDisk].diskIp);
    write(socket, buff, 1024);
    
}

void _download(const char* userFilename, int socket, int partition, char *login_name, vector<int> &partitionArray, vector<Disk> &DiskList, vector<string> &DPAHelper, set<int> &userFileHashSet) {
    char username[10], filename[20], tempFilename[30];
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

    char buff[1024];
    vector<string>::iterator dpaIt;
    dpaIt = find(DPAHelper.begin(), DPAHelper.end(), userFilename);
    if (dpaIt == DPAHelper.end()) {
        strcpy(buff, "File not found!");
        write(socket, buff, sizeof(buff));
        return;
    }
    else {
        strcpy(buff, "Start downloading...");
        write(socket, buff, sizeof(buff));
    }

    string userFileHash = md5_hash(userFilename, partition);
    int hashValue = stoi(userFileHash, nullptr, 2);
    
    int mainDisk = partitionArray[hashValue];
    int backupDisk = ((partitionArray[hashValue] == DiskList.size() - 1)? 0 : partitionArray[hashValue] + 1);

    RestoreFiles(login_name, username, filename, DiskList, backupDisk, mainDisk);

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

    char filePath[MAX_PATHLENGTH];
    strcpy(filePath, directory);
    strcat(filePath, filename);

    ifstream fileToDownload(filePath, ios::binary);
    while (fileToDownload.read(buff, sizeof(buff))) {
        write(socket, buff, sizeof(buff));
    }
    write(socket, "EOF", 4);
    fileToDownload.close();

    // Clean up
    remove(filePath);
    rmdir(directory);
    cout << string(username) << "/" << string(filename) << " was downloaded from " << DiskList[mainDisk].diskIp << endl;
}

void _list(const char* username, int socket, int partition, char *login_name, vector<int> &partitionArray, vector<Disk> &DiskList, vector<string> &DPAHelper, set<int> &userFileHashSet) {
    ofstream allFiles("allFiles.sh");
    if (!allFiles.is_open()) {
        cerr << "Error: Unable to open allFiles.sh" << endl;
        return;
    }
    allFiles.close();
    system("chmod 700 allFiles.sh");

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
    }
    
    for (int mainDisk = 0; mainDisk < DiskList.size(); mainDisk++) {
        string diskIp = DiskList[mainDisk].diskIp;
        vector<string> filelist(DiskList[mainDisk].fileList);

        for (string LoginUserFile : filelist) {
            if (LoginUserFile.find(username) != string::npos && LoginUserFile.find("backupFolder") == string::npos) {
                string _filename = LoginUserFile.substr(LoginUserFile.find_last_of('/') + 1);
                char filename[20];
                strcpy(filename, _filename.c_str());
                string userFilename = LoginUserFile.substr(LoginUserFile.find_first_of('/') + 1);
                string userFileHash = md5_hash(userFilename, partition);
                int hashValue = stoi(userFileHash, nullptr, 2);

                int backupDisk = ((partitionArray[hashValue] == DiskList.size() - 1)? 0 : partitionArray[hashValue] + 1);

                RestoreFiles(login_name, username, filename, DiskList, backupDisk, mainDisk);

                string GetDiskList = "ssh -o StrictHostKeyChecking=no " + string(login_name) + "@" + DiskList[mainDisk].diskIp + " \"cd /tmp/" + string(login_name) + "/" + string(username) + " ; " + "ls -lrt >> ~/output" + DiskList[mainDisk].diskIp + ".txt\"";
                string CopyFile = "scp -B " + string(login_name) + "@" + DiskList[mainDisk].diskIp + ":~/output" + DiskList[mainDisk].diskIp + ".txt " + string(directory) + "output" + DiskList[mainDisk].diskIp + ".txt";
                string RemoveOutput = "ssh -o StrictHostKeyChecking=no " + string(login_name) + "@" + DiskList[mainDisk].diskIp + " \"rm ~/output" + DiskList[mainDisk].diskIp + ".txt\"";
                
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

    // string outputDirectory = string(directory) + "output.txt";
    ofstream outfile("list_" + string(username) + "_files.txt", ios::binary);
    if (!outfile.is_open()) {
        cerr << "Error: Unable to open output file for writing" << endl;
        return;
    }

    for (int mainDisk = 0; mainDisk < DiskList.size(); mainDisk++) {
        string diskIp = DiskList[mainDisk].diskIp;
        string entryPath = string(directory) + "output" + diskIp + ".txt";

        ifstream infile(entryPath);
        if (infile.is_open()) {
            // cout << "Copying contents from " << entryPath << endl;
            outfile << infile.rdbuf();
            infile.close();
            remove(entryPath.c_str());
        }
    }
    outfile.close();

    /*
    ifstream f(outputDirectory, ios::binary);
    if (!f.is_open()) {
        cerr << "Error: Unable to open output.txt for reading" << endl;
        return;
    }

    f.seekg(0, ios::end);
    int filesize = f.tellg();
    f.seekg(0, ios::beg);

    char buffer[1024];
    while (f.read(buffer, 1024)) {
        cout << "Sending" << endl;
        write(socket, buffer, sizeof(buffer));
    }

    f.close();
    */

    //remove(outputDirectory.c_str());
    remove(directory);

    string completedMessage = "Listing was completed and saved to file list_" + string(username) + "_files.txt";
    cout << completedMessage << endl;
    write(socket, completedMessage.c_str(), completedMessage.length());
}

void _delete(const char* userFilename, int socket, int partition, char *login_name, vector<int> &partitionArray, vector<Disk> &DiskList, vector<string> &DPAHelper, set<int> &userFileHashSet) {
    // Check if the file exists
    char buff[1024];
    vector<string>::iterator dpaIt;
    dpaIt = find(DPAHelper.begin(), DPAHelper.end(), userFilename);
    if (dpaIt == DPAHelper.end()) {
        strcpy(buff, "File not found!");
        write(socket, buff, sizeof(buff));
        return;
    } else {
        strcpy(buff, "Start deleting...");
        write(socket, buff, sizeof(buff));
    }
    // Split UserFileName into username and filename
    char username[10], filename[20], tempFilename[30];
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

    string userFileHash = md5_hash(userFilename, partition);
    int hashValue = stoi(userFileHash, nullptr, 2);
    
    int mainDisk = partitionArray[hashValue];
    int backupDisk = ((partitionArray[hashValue] == DiskList.size() - 1)? 0 : partitionArray[hashValue] + 1);

    // Delete file from main disk
    ofstream commandfile("commandfile.sh");
    if (!commandfile.is_open()) {
        cerr << "Error: Unable to open commandfile.sh" << endl;
        return;
    }
    commandfile << "ssh -o StrictHostKeyChecking=no " << login_name << "@" << DiskList[mainDisk].diskIp << " \"cd /tmp/" << login_name << "/" << username << " ; rm " << filename << "\"" << endl;
    commandfile.close();
    system("sh commandfile.sh");
    remove("commandfile.sh");

    // Delete file from backup disk
    commandfile.open("commandfile.sh");
    if (!commandfile.is_open()) {
        cerr << "Error: Unable to open commandfile.sh" << endl;
        return;
    }
    commandfile << "ssh -o StrictHostKeyChecking=no " << login_name << "@" << DiskList[backupDisk].diskIp << " \"cd /tmp/" << login_name << "/backupfolder/" << username << " ; rm " << filename << "\"" << endl;
    commandfile.close();
    system("sh commandfile.sh");
    remove("commandfile.sh");

    // Delete file record in DiskList
    for (Disk _mainDisk : DiskList) {
        vector<string> filelist(_mainDisk.fileList);
        for (auto it = filelist.begin(); it != filelist.end(); ) {
            if (*it == userFilename) {
                it = filelist.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Delete file record in UserFileHashSet
    userFileHashSet.erase(hashValue);

    // Delete file record in DPAHelper
    DPAHelper[hashValue] = "";

    // Send result back to client
    sprintf(buff, "%s/%s was deleted from main disk %s and backup disk: %s.", username, filename, DiskList[mainDisk].diskIp, DiskList[backupDisk].diskIp);
    write(socket, buff, 1024);
}

void _add(const char *newDiskIp, int socket, int partition, char *login_name, vector<int> &partitionArray, vector<Disk> &DiskList, vector<string> &DPAHelper, set<int> &userFileHashSet, map<string, int> &diskIndex) {
    Disk newDisk;
    strcpy(newDisk.diskIp, newDiskIp);
    DiskList.push_back(newDisk);
    int l = DiskList.size();

    diskIndex[newDiskIp] = diskIndex.size();
    int l2 = diskIndex.size();

    for (int j = 0; j < l2; ++j) {
        int count = 0;
        for (int i = 0; i < partitionArray.size(); ++i) {
            if (partitionArray[i] == j && count < (2 << partition)/l - 1) {
                // Move backup files from DiskList[0] to DiskList[l - 1]
                if (userFileHashSet.count(i) && j == (l - 2)) {
                    string username = DPAHelper[i].substr(0, DPAHelper[i].find("/"));
                    string filename = DPAHelper[i].substr(DPAHelper[i].find("/") + 1);

                    DownloadUploadForBackup(login_name, username, filename, DiskList, 0, l - 1);
                    DeleteForBackup(login_name, username, filename, DiskList, 0, l - 1);
                    
                    char filePath[MAX_PATHLENGTH];
                    sprintf(filePath, "%s/backupFolder/%s/%s", login_name, username, filename);
                    DiskList[l - 1].fileList.push_back(filePath);
                    vector<string> filelist = DiskList[0].fileList;
                    for (auto it = filelist.begin(); it != filelist.end(); ) {
                        if (it->find("/backupfolder/" + username + "/" + filename) != string::npos) {
                            it = filelist.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
                count++;
            } else if (partitionArray[i] == j) {
                partitionArray[i] = diskIndex[newDiskIp];
                if (userFileHashSet.count(i)) {
                    string username = DPAHelper[i].substr(0, DPAHelper[i].find("/"));
                    string filename = DPAHelper[i].substr(DPAHelper[i].find("/") + 1);

                    int downMainDisk = j;
                    int upMainDisk = diskIndex[newDiskIp];

                    int downloadBackupDisk = ((j == l - 2)? 0 : (j + 1));
                    int uploadBackupDisk = 0;

                    DownloadUpload(login_name, username, filename, DiskList, downMainDisk, upMainDisk);
                    Delete(login_name, username, filename, DiskList, downMainDisk, upMainDisk);

                    DownloadUploadForBackup(login_name, username, filename, DiskList, downloadBackupDisk, uploadBackupDisk);
                    Delete(login_name, username, filename, DiskList, downloadBackupDisk, uploadBackupDisk);
                    
                    char filePath[MAX_PATHLENGTH];
                    sprintf(filePath, "%s/backupFolder/%s/%s", login_name, username, filename);
                    DiskList[upMainDisk].fileList.push_back(filePath);
                    vector<string> filelist = DiskList[downMainDisk].fileList;
                    for (auto it = filelist.begin(); it != filelist.end(); ) {
                        if (it->find(username + "/" + filename) != string::npos) {
                            it = filelist.erase(it);
                        } else {
                            ++it;
                        }
                    }

                    sprintf(filePath, "%s/backupFolder/%s/%s", login_name, username, filename);
                    DiskList[uploadBackupDisk].fileList.push_back(filePath);
                    vector<string> backupFileList = DiskList[downloadBackupDisk].fileList;
                    for (auto it = backupFileList.begin(); it != backupFileList.end(); ) {
                        if (it->find("/backupfolder/" + username + "/" + filename) != string::npos) {
                            it = backupFileList.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
                count++;
            }
        }
    }
    
    // Send result back to client
    char result[1024];
    strcpy(result, "All files are now in disks");
    for (Disk _d : DiskList) {
        strcat(result, " ");
        strcat(result, _d.diskIp);
    }
    strcat(result, ".");
    write(socket, result, sizeof(result));

    return;
}

void _remove(const char *oldDiskIp, int socket, int partition, char *login_name, vector<int> &partitionArray, vector<Disk> &DiskList, vector<string> &DPAHelper, set<int> &userFileHashSet, map<string, int> &diskIndex) {
    vector<int> oldPartitionArray(partitionArray);

    // Check if OldDisk exists
    int oldDisk = -1;
    for (int i = 0; i < DiskList.size(); ++i) {
        if (strcmp(DiskList[i].diskIp, oldDiskIp) == 0) {
            oldDisk = i;
            break;
        }
    }
    if (oldDisk == -1) {
        cout << "Invalid disk IP!" << endl;
        return;
    }

    int l = DiskList.size();
    int l2 = diskIndex.size();

    // Remove old disk record in partitionArray
    for (int j = 0; j < l2; ++j) {
        int count = 0;
        for (int i = 0; i < partitionArray.size(); ++i) {
            if (partitionArray[i] == j) {
                count++;
            }
        }
        for (int k = 0; k < partitionArray.size(); ++k) {
            if (partitionArray[k] == oldDisk && count < (2 << partition)/(l - 1)) {
                partitionArray[k] = diskIndex[DiskList[j].diskIp];
                count++;
            }
        }
    }

    for (int h = 0; h < partitionArray.size(); ++h) {
        if (oldPartitionArray[h] == oldDisk - 1 && userFileHashSet.count(h)) {
            string username = DPAHelper[h].substr(0, DPAHelper[h].find("/"));
            string filename = DPAHelper[h].substr(DPAHelper[h].find("/") + 1);
            
            char filePath[MAX_PATHLENGTH];
            sprintf(filePath, "%s/backupFolder/%s/%s", login_name, username, filename);
            DiskList[oldDisk + 1].fileList.push_back(filePath);
        }

        if (oldPartitionArray[h] == oldDisk && userFileHashSet.count(h)) {
            string username = DPAHelper[h].substr(0, DPAHelper[h].find("/"));
            string filename = DPAHelper[h].substr(DPAHelper[h].find("/") + 1);
            int downMainDisk = oldDisk;
            int upMainDisk = partitionArray[h];
            int downloadBackupDisk = ((partitionArray[h] == (l - 1))? 0 : oldDisk + 1);
            int uploadBackupDisk = ((partitionArray[h] == (oldDisk - 1))? partitionArray[h] + 2 : partitionArray[h] + 1);

            char filePath[MAX_PATHLENGTH];
            if (strcmp(DiskList[upMainDisk].diskIp, DiskList[downMainDisk].diskIp) != 0) {
                sprintf(filePath, "%s/%s/%s", login_name, username, filename);
                DiskList[upMainDisk].fileList.push_back(filePath);
            }
            if (strcmp(DiskList[uploadBackupDisk].diskIp, DiskList[downloadBackupDisk].diskIp) != 0) {
                sprintf(filePath, "%s/backupFolder/%s/%s", login_name, username, filename);
                DiskList[uploadBackupDisk].fileList.push_back(filePath);
            }
        }
    }

    DiskList.erase(DiskList.begin() + oldDisk);

    // Send result back to client
    char result[1024];
    strcpy(result, "All files are now in disks");
    for (Disk _d : DiskList) {
        strcat(result, " ");
        strcat(result, _d.diskIp);
    }
    strcat(result, ".");
    write(socket, result, sizeof(result));

    return;
}

void _clean(int socket, int partition, char *login_name, vector<int> &partitionArray, vector<Disk> &DiskList, vector<string> &DPAHelper, set<int> &userFileHashSet, map<string, int> &diskIndex) {
    partitionArray.clear();
    for (int _disk = 0; _disk < DiskList.size(); _disk++) {
        DiskList[_disk].fileList.clear();
        string clearCommand = "ssh -o StrictHostKeyChecking=no " + string(login_name) + "@" + string(DiskList[_disk].diskIp)
            + " \"rm -rf /tmp/" + string(login_name) + "\"";
        system(clearCommand.c_str());
    }
    for (int hashSet : userFileHashSet) {
        DPAHelper[hashSet] = "";
    }
    userFileHashSet.clear();
    diskIndex.clear();
    char buff[1024];
    sprintf(buff, "All disks have been cleared.");
    write(socket, buff, 1024);
}

int main(int argc, char *argv[]) {
    // Declare socket file descriptor.
    int sockfd, connfd;

    // Declare server address to which to bind for receiving messages and client address to fill in sending address
    struct sockaddr_in servAddr, clienAddr;

    // Get from the command line, server IP, src and dst files.
    if (argc <= 2) {
        cerr << "Usage: " << argv[0] << " <# partition power> <available disks> " << endl;
        exit(0);
    }

    int partition = atoi(argv[1]);
    int numDisks = argc - 2;

    vector<Disk> DiskList;
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

    int numPartition = (2 << partition);
    vector<int> partitionArray(numPartition);
    vector<string> DPAHelper(numPartition);

    for (int pNum = 0; pNum < numPartition; pNum++) {
        for (int i = 0; i < numDisks; i++) {
            if (pNum >= numPartition * i / numDisks && pNum < numPartition * (i + 1) / numDisks) {
                partitionArray[pNum] = i;
            }
        }
    }

    set<int> userFileHashSet;

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
    char cmdInput[40], command[10], arg[30];
    while (true) {
        if ((connfd = accept(sockfd, (struct sockaddr *)&clienAddr, (socklen_t *)&sin_size)) < 0) {
            perror("Failure to accept connection to the client");
            exit(0);
        }
        cout << "Connection Established with client IP: " << inet_ntoa(clienAddr.sin_addr) << " and Port: " << ntohs(clienAddr.sin_port) << endl;
        ssize_t bytes_read = read(connfd, buff, sizeof(buff) - 1); // recv 1
         // -1 to leave space for null terminator
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

        if (strcmp(command, "upload") == 0) {
            _upload(arg, connfd, partition, login_name, partitionArray, DiskList, DPAHelper, userFileHashSet);
        } else if (strcmp(command, "download") == 0) {
            _download(arg, connfd, partition, login_name, partitionArray, DiskList, DPAHelper, userFileHashSet);
        } else if (strcmp(command, "list") == 0) {
            _list(arg, connfd, partition, login_name, partitionArray, DiskList, DPAHelper, userFileHashSet);
        } else if (strcmp(command, "delete") == 0) {
            _delete(arg, connfd, partition, login_name, partitionArray, DiskList, DPAHelper, userFileHashSet);
        } else if (strcmp(command, "add") == 0) {
            _add(arg, connfd, partition, login_name, partitionArray, DiskList, DPAHelper, userFileHashSet, diskIndex);
            printDiskList(DiskList);
        } else if (strcmp(command, "remove") == 0) {
            _remove(arg, connfd, partition, login_name, partitionArray, DiskList, DPAHelper, userFileHashSet, diskIndex);
            printDiskList(DiskList);
        } else if (strcmp(command, "clean") == 0) {
            _clean(connfd, partition, login_name, partitionArray, DiskList, DPAHelper, userFileHashSet, diskIndex);
        } else {
            cout << "Invalid command!" << endl;
        }
    }
    // Close socket descriptor
    close(sockfd);

    return 0;
}

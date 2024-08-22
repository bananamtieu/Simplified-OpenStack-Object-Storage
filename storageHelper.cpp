#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <set>
#include <vector>
#include <map>
#include <bits/stdc++.h>
#include "storageHelper.h"

using namespace std;

void RestoreFiles(const char *LoginName, const char *username, const char *filename,
    vector<Disk> DiskList, int BackupDisk, int MainDisk) {
    // Restore from BackupDisk
    // Download
    const string directory = "/tmp/tempFile/";
    if (mkdir(directory.c_str(), 0777) != 0 && errno != EEXIST) {
        cerr << "Error: Failed to create directory." << endl;
        return;
    }

    ofstream commandfile("commandfile.sh");
    commandfile << "#!/bin/bash" << endl;
    commandfile << "scp -B " << LoginName << "@" << DiskList[BackupDisk].diskIp << ":/tmp/" << LoginName << "/backupfolder/"
                << username << "/" << filename << " " << directory << filename << endl;
    commandfile.close();
    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    // Upload
    commandfile.open("commandfile.sh");
    commandfile << "#!/bin/bash" << endl;
    commandfile << "scp -B " << directory << filename << " " << LoginName << "@" << DiskList[MainDisk].diskIp << ":/tmp/"
                << LoginName << "/" << username << "/" << filename << endl;
    commandfile.close();
    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    // Restore from MainDisk
    // Download
    commandfile.open("commandfile.sh");
    commandfile << "#!/bin/bash" << endl;
    commandfile << "scp -B " << LoginName << "@" << DiskList[MainDisk].diskIp << ":/tmp/" << LoginName << "/"
                << username << "/" << filename << " " << directory << filename << endl;
    commandfile.close();
    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    // Upload
    commandfile.open("commandfile.sh");
    commandfile << "#!/bin/bash" << endl;
    commandfile << "scp -B " << directory << filename << " " << LoginName << "@" << DiskList[BackupDisk].diskIp
                << ":/tmp/" << LoginName << "/backupfolder/" << username << "/" << filename << endl;
    commandfile.close();
    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    // Remove temporary directory
    rmdir(directory.c_str());
}

void moveMain(const string& LoginName, const string& username, const string& filename, vector<Disk> DiskList, int oldMainDisk, int newMainDisk) {
    // Download
    string directory = "/tmp/tempFile/";
    if (system(("mkdir -p " + directory).c_str()) != 0) {
        cerr << "Error creating directory!" << endl;
        return;
    }

    ofstream commandfile("commandfile.sh");
    commandfile << "#!/bin/bash" << endl;
    string CopyFile = "scp -B " + LoginName + "@" + DiskList[oldMainDisk].diskIp +":/tmp/" + LoginName + "/" + username + "/" + filename + " " + directory + filename;
    commandfile << CopyFile << endl;
    commandfile.close();

    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    // Upload
    ofstream commandfileUpload("commandfile.sh");
    commandfileUpload << "#!/bin/bash" << endl;

    string folderCreate = "ssh -o StrictHostKeyChecking=no " + LoginName+"@" + DiskList[newMainDisk].diskIp + " \"mkdir -p /tmp/" + LoginName + "/" + username + "\"";
    string CopyFileUpload = "scp -B " + directory + filename + " " + LoginName + "@" + DiskList[newMainDisk].diskIp + ":/tmp/" + LoginName + "/" + username + "/" + filename;
    commandfileUpload << folderCreate << endl;
    commandfileUpload << CopyFileUpload << endl;
    commandfileUpload.close();

    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    remove((directory + filename).c_str());
    remove(directory.c_str());
}

void deleteOldMain(const string& LoginName, const string& username, const string& filename, vector<Disk> DiskList, int oldMainDisk) {
    // Delete
    ofstream commandfile("commandfile.sh");
    commandfile << "#!/bin/bash" << endl;

    string DeleteFile = "ssh -o StrictHostKeyChecking=no "+ LoginName + "@" + DiskList[oldMainDisk].diskIp +" \"cd /tmp/" + LoginName + "/" + username + " ; " + "rm " + filename + "\"";
    commandfile << DeleteFile << endl;
    commandfile.close();

    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");
}

void moveBackup(const string& LoginName, const string& username, const string& filename, vector<Disk> DiskList, int oldBackupDisk, int newBackupDisk) {
    // Download
    string directory = "/tmp/tempFile/";
    if (system(("mkdir -p " + directory).c_str()) != 0) {
        cerr << "Error creating directory!" << endl;
        return;
    }

    ofstream commandfile("commandfile.sh");
    commandfile << "#!/bin/bash" << endl;
    
    string CopyFile = "scp -B " + LoginName + "@" + DiskList[oldBackupDisk].diskIp + ":/tmp/" + LoginName + "/backupfolder/" + username + "/" + filename + " " + directory + filename;
    commandfile << CopyFile << endl;
    commandfile.close();

    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    // Upload
    ofstream commandfileUpload("commandfile.sh");
    commandfileUpload << "#!/bin/bash" << endl;
    
    string folderCreateB = "ssh -o StrictHostKeyChecking=no " + LoginName + "@" + DiskList[newBackupDisk].diskIp + " \"mkdir -p /tmp/" + LoginName + "/backupfolder" + "\"";
    string folderCreate = "ssh -o StrictHostKeyChecking=no " + LoginName + "@" + DiskList[newBackupDisk].diskIp +" \"mkdir -p /tmp/" + LoginName + "/backupfolder/" + username + "\"";
    string CopyFileUpload = "scp -B " + directory + filename + " " + LoginName + "@" + DiskList[newBackupDisk].diskIp + ":/tmp/" + LoginName + "/backupfolder/" + username + "/" + filename;
    commandfileUpload << folderCreateB << endl;
    commandfileUpload << folderCreate << endl;
    commandfileUpload << CopyFileUpload << endl;
    commandfileUpload.close();

    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");

    remove((directory + filename).c_str());
    remove(directory.c_str());
}

void deleteOldBackup(const string& LoginName, const string& username, const string& filename, vector<Disk> DiskList, int oldBackupDisk) {
    // Delete
    ofstream commandfile("commandfile.sh");
    commandfile << "#!/bin/bash" << endl;
    
    string DeleteFile = "ssh -o StrictHostKeyChecking=no " + LoginName + "@" + DiskList[oldBackupDisk].diskIp + " \"cd /tmp/" + LoginName + "/backupfolder/" + username + " ; " + "rm " + filename + "\"";
    commandfile << DeleteFile << endl;
    commandfile.close();

    chmod("commandfile.sh", 0700);
    system("./commandfile.sh");
    remove("commandfile.sh");
}
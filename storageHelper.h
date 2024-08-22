#ifndef STORAGEHELPER_H
#define STORAGEHELPER_H

#define MAX_LOGIN_NAME 30

using namespace std;

struct Disk {
    string diskIp;
    vector<string> fileList;
};

void RestoreFiles(const char *LoginName, const char *username, const char *filename, vector<Disk> DiskList, int BackupDisk, int MainDisk);
void moveMain(const string& LoginName, const string& username, const string& filename, vector<Disk> DiskList, int oldMainDisk, int newMainDisk);
void deleteOldMain(const string& LoginName, const string& username, const string& filename, vector<Disk> DiskList, int oldMainDisk);
void moveBackup(const string& LoginName, const string& username, const string& filename, vector<Disk> DiskList, int oldBackupDisk, int newBackupDisk);
void deleteOldBackup(const string& LoginName, const string& username, const string& filename, vector<Disk> DiskList, int oldBackupDisk);

#endif
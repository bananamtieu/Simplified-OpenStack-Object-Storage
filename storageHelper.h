#ifndef STORAGEHELPER_H
#define STORAGEHELPER_H

#define MAX_LOGIN_NAME 30
#define MAX_DISKNAMELENGTH 20
#define MAX_PATHLENGTH 80

using namespace std;

struct Disk {
    char diskIp[MAX_DISKNAMELENGTH];
    vector<string> fileList;
};

void RestoreFiles(const char *LoginName, const char *username, const char *filename, vector<Disk> DiskList, int BackupDisk, int MainDisk);
void DownloadUpload(const string& LoginName, const string& username, const string& filename, vector<Disk> DiskList, int dMainDisk, int uMainDisk);
void Delete(const string& LoginName, const string& username, const string& filename, vector<Disk> DiskList, int dMainDisk, int uMainDisk);
void DownloadUploadForBackup(const string& LoginName, const string& username, const string& filename, vector<Disk> DiskList, int dBackupDisk, int uBackupDisk);
void DeleteForBackup(const string& LoginName, const string& username, const string& filename, vector<Disk> DiskList, int dBackupDisk, int uBackupDisk);

#endif
#ifndef SHARED_CONSTANTS_H
#define SHARED_CONSTANTS_H


int TASK_REPLACE = 1;
int TASK_R1_STORAGE = 2;
int TASK_R5_STORAGE = 3;
int TASK_GETATTR = 4;
int TASK_ACCESS = 5;
int TASK_READLINK = 6;
int TASK_READDIR = 7;
int TASK_MKNOD = 8;
int TASK_UNLINK = 9;
int TASK_RMDIR = 10;
int TASK_CHMOD = 11;
int TASK_TRUNCATE = 12;
int TASK_OPEN = 13;
int TASK_READ = 14;
int TASK_WRITE = 15;
int TASK_RELEASE = 16;
int TASK_MKDIR = 17;
int TASK_RENAME = 18;
int TASK_CPYFL = 19;
int TASK_RCVFL = 20;
int TASK_HEALTHCHECK = 21;

#define MAX_PATH 512
#define MAX_BUF 8192

char path_to_fuse_R1[MAX_PATH] = "/home/vagrant/Net-Raid-FileSystem/Fuse/fusecxR1";
char REALITY_PATH[MAX_PATH] = "/home/vagrant/Net-Raid-FileSystem/Fuse/reality";


#endif

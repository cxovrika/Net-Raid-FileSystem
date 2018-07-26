#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H
#include "shared_constants.h"
#include <sys/stat.h>

struct initial_task {
  int task_type;
};

struct server_and_port {
  char server[32];
  int port;
};

struct storage_info {
  char diskname[32];
  char mountpoint[256];
  int raid;
  int server_count;
  struct server_and_port servers[32];
  struct server_and_port hotswap;
};

struct task_R1 {
  char comment[64];
  int task_type;
  char path[MAX_PATH];
  char buf[MAX_BUF];
  int size;
  int offset;
  int mask;
  mode_t mode;
  dev_t rdev;
  char from[MAX_PATH];
  char to[MAX_PATH];
  int flags;
};

struct server_response_R1 {
  char comment[30];
  int ret_val;
  struct stat stbuf;
  char buf[8192];
  int files_in_dir;
  char file_names[64][32];
  struct stat stats[32];
};


#endif

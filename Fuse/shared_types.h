#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

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

#endif

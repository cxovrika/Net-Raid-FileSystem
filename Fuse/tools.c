#ifndef TOOLS_C
#define TOOLS_C

#include "net_raid_client.h"
#include "net_raid_server.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int get_next_token(int fd, char* token) {
  int ret = 0;
  char c;
  while (1) {
    int r = read(fd, &c, 1);
    if (r == 0) goto final;
    if (c == '\n' || c == '\r' || c == ' ') continue;

    token[ret++] = c;
    break;
  }

  while (1) {
    int r = read(fd, &c, 1);
    if (r == 0) goto final;
    if (c == '\n' || c == '\r' || c == ' ') goto final;

    token[ret++] = c;
  }

  final:
  token[ret] = '\0';
  return ret;
}


void fill_non_storage_data(int config_fd) {
  char token[256];
  //errorlog
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  strcpy(path_to_errorlog, token);

  //cache_size
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  cache_size = atoi(token);

  //cache_replacement
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);

  //timeout
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  timeout = atoi(token);
}

struct server_and_port get_server_and_port_from_token(char* token) {
  struct server_and_port sap;
  int i = 0;
  for (; token[i] != ':'; i++);
  strncpy(sap.server, token, i);
  sap.port = atoi(token + (i+1));

  return sap;
}

int get_next_storage(struct storage_info* si, int config_fd) {
  char token[256];

  //diskname
  if (!get_next_token(config_fd, token)) return 0;
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  strcpy(si->diskname, token);

  //mountpoint
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  strcpy(si->mountpoint, token);

  //raid
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  si->raid = atoi(token);

  //servers
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  while(1) {
      get_next_token(config_fd, token);
      if (token[0] == 'h') break; //shitty code but works with the given format :V
      si->servers[si->server_count++] = get_server_and_port_from_token(token);
  }

  //hotswap
  get_next_token(config_fd, token);
  get_next_token(config_fd, token);
  si->hotswap = get_server_and_port_from_token(token);

  return 1;
}

void fill_storage_data(int config_fd) {

  while (1) {
    struct storage_info si;
    memset(&si, 0, sizeof(si));

    si.server_count = 0;
    if (!get_next_storage(&si, config_fd))
      break;

    storages[storage_count++] = si;
  }
}

void get_configurations(int config_fd) {
  fill_non_storage_data(config_fd);
  fill_storage_data(config_fd);
}

void read_from_socket(int socket, char* buf, size_t len, int flags) {

  char ch;
  while (len > 0)
  {
      int ret = recv(socket, &ch, 1, flags);
      if (ret > 0)
      {
          *buf = ch;
          ++buf;
          len--;
      }
  }

}

#endif

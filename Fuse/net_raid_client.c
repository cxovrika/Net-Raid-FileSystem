#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "shared_types.h"
#include "shared_constants.h"
#include "net_raid_client.h"
#include "md5.h"

#include "tools.c"


void parse_and_fill_parameters(int argc, char* argv[]) {
  strcpy(path_to_config, argv[1]);
  printf("Opening configurations file from path: %s\n", path_to_config);
  config_fd = open(path_to_config, 0);
}

void run_new_storage(int x) {
  struct storage_info si = storages[x];
  int argc = (5 + 2*(si.server_count + 1) + 1);
  char** parameters = malloc(argc * sizeof(char*));
  parameters[0] = malloc(256);
  parameters[1] = malloc(256);
  parameters[2] = malloc(256);
  parameters[3] = malloc(256);
  parameters[4] = malloc(256);

  strcpy(parameters[0], si.mountpoint);
  strcpy(parameters[1], path_to_errorlog);
  strcpy(parameters[2], si.diskname);
  strcpy(parameters[3], timeout);
  strcpy(parameters[4], cache_size);

  for (int i = 0; i < si.server_count; i++) {
    //server
    parameters[2*i + 5] = malloc(32);
    strcpy(parameters[2*i + 5], si.servers[i].server);

    //port
    parameters[2*i + 6] = malloc(32);
    sprintf(parameters[2*i + 6], "%d", si.servers[i].port);
  }

  //hotswap
  parameters[2*si.server_count + 5] = malloc(32);
  strcpy(parameters[2*si.server_count + 5], si.hotswap.server);

  parameters[2*si.server_count + 6] = malloc(32);
  sprintf(parameters[2*si.server_count + 6], "%d", si.hotswap.port);

  //terminating NULL (needed for execv)
  parameters[argc-1] = NULL;

  execv(path_to_fuse_R1, parameters);
}

int main(int argc, char* argv[])
{
    parse_and_fill_parameters(argc, argv);
    if (config_fd == -1) {
      printf("Can't open configurations file\n");
      return -1;
    }

    get_configurations(config_fd);


    for (int i = 0; i < storage_count; i++) {
      switch(fork()) {
        case -1:
          exit(100);

        case 0:
          run_new_storage(i);
          break;

        default:
          break;
      }

    }

    return 0;
}

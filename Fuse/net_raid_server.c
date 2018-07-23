#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include "shared_types.h"
#include "shared_constants.h"
#include "net_raid_server.h"

#include "tools.c"


void bring_server_up() {
  printf("Bringing server up...\n");
  // server socket creation
  server_socket = socket(AF_INET, SOCK_STREAM, 0);

  // server address definitoin
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  server_address.sin_addr.s_addr = inet_addr(client_ip);

  // bind socket to specified port
  bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));
  listen(server_socket, BACKLOG);
}

void parse_and_fill_parameters(int argc, char* argv[]) {
  printf("Parsing arguments...\n");
  strcpy(client_ip, argv[1]);
  PORT = atoi(argv[2]);
  strcpy(path_to_storage, argv[3]);
  // printf("ip: %s\nport: %d\npath: %s\n", client_ip, PORT, path_to_storage \);
}

void accept_and_serve_clienet() {
  printf("Accepting client...\n");
  // accepting client
  client_socket = accept(server_socket, NULL, NULL);
  printf("Accepted!\n");

  struct initial_task it;
  recv(client_socket, &it, sizeof(it), 0);

  printf("initial_task: %d\n", it.task_type);

  // closing connection
  close(server_socket);
}

int main(int argc, char* argv[]) {
  parse_and_fill_parameters(argc, argv);
  bring_server_up();
  accept_and_serve_clienet();
}

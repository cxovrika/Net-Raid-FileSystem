#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <netinet/in.h>
#include "shared_types.h"
#include "shared_constants.h"
#include "net_raid_server.h"
#include "md5.h"


#include "tools.c"

#include "task_handling_R1.c"

static struct epoll_event ev;
int epfd;

void bring_server_up() {
  // printf("Bringing server up...\n");
  // server socket creation
  server_socket = socket(AF_INET, SOCK_STREAM, 0);

  //xz
  int optval = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

  // server address definitoin
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  server_address.sin_addr.s_addr = inet_addr(client_ip);

  // bind socket to specified port
  bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));
  listen(server_socket, BACKLOG);
}

void parse_and_fill_parameters(int argc, char* argv[]) {
  // printf("Parsing arguments...\n");
  strcpy(client_ip, argv[1]);
  PORT = atoi(argv[2]);
  strcpy(path_to_storage, argv[3]);
  // printf("ip: %s\nport: %d\npath: %s\n", client_ip, PORT, path_to_storage \);
}

void serve_client_for_R1() {
  struct task_R1 task;
  while (1) {
    int nfds = epoll_wait(epfd, &ev, 1, -1);
    int bytes_read = recv(ev.data.fd, &task, sizeof(task), 0);
    if (bytes_read == 0) break;
    handle_task_R1(task);
  }
}


void accept_and_serve_clienet() {
  // printf("Accepting client...\n");
  // accepting client
  client_socket = accept(server_socket, NULL, NULL);
  // printf("Accepted!\n");

  // printf("creating epoll\n");
  epfd = epoll_create(1);
  ev.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
  ev.data.fd = client_socket;
  int res = epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &ev);
  int nfds = epoll_wait(epfd, &ev, 1, -1);

  struct initial_task it;
  recv(ev.data.fd, &it, sizeof(it), 0);

  // printf("initial_task: %d\n", it.task_type);

  if (it.task_type == TASK_R1_STORAGE) {
    serve_client_for_R1();
  } else {
    // closing connection
    close(server_socket);
  }

}

int main(int argc, char* argv[]) {
  parse_and_fill_parameters(argc, argv);
  bring_server_up();
  accept_and_serve_clienet();
}

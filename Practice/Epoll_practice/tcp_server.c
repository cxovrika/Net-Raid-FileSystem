#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>

int main() {
  char server_message[256] = "I am groot";

  // server socket creation
  int server_socket;
  server_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (server_socket == -1) {
    printf("socket problem\n");
    printf("socket error: %s\n", strerror(errno));
  }
  // server address definitoin
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(9011); //ITS OVER 9000 AGAIN!!!!
  server_address.sin_addr.s_addr = INADDR_ANY;

  // bind sockdt to specified IP and port
  if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
    printf("bind problem\n");
    printf("bind error: %s\n", strerror(errno));
  }

  if (listen(server_socket, 5) == -1) {
    printf("listen problem\n");
    printf("listen error: %s\n", strerror(errno));
  }

  int epfd = epoll_create(1);
  printf("created eppoll: %d\n", epfd);

  // accepting client
  int client_socket;
  printf("accepting client\n");
  client_socket = accept(server_socket, NULL, NULL);
  printf("accepted\n");

  static struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
  ev.data.fd = client_socket;
  int res = epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &ev);


  printf("epoll is waiting:\n");
  int nfds = epoll_wait(epfd, &ev, 1, -1);
  printf("epoll woke up\n");

  read(ev.data.fd, &server_message, sizeof(server_message));
  printf("recieved: %s\n", server_message);

  // sending message
  send(ev.data.fd, server_message, sizeof(server_message), 0);

  // closing connection
  close(server_socket);

  return 0;
}

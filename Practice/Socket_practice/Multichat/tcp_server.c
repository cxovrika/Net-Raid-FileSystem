#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

struct sockaddr_in server_address;
int server_socket;
int client_sockets[1000];
int client_count = 0;

char server_message[256] = "Enter nickname:";

void* serve_client(void* client_socket_) {
  printf("Accepted someone\n");
  char buf[256];
  int client_socket = *((int*)client_socket_);

  // Nickname asking
  char nickname[256];
  read(client_socket, nickname, sizeof(nickname));
  char join_message[256];
  strcpy(join_message, nickname);
  strcat(join_message, " joined the room");
  printf("%s\n", join_message);

  // join notification
  for (int i = 0; i < client_count; i++) {
    if (client_sockets[i] == client_socket) continue;
    write(client_sockets[i], join_message, sizeof(join_message));
  }

  while (1) {
    read(client_socket, &buf, sizeof(buf));
    printf("recieved: %s\n", buf);
    char msg[256];
    strcpy(msg, nickname);
    strcat(msg, ": ");
    strcat(msg, buf);
    printf("msg: %s\n", msg);

    for (int i = 0; i < client_count; i++) {
      if (client_sockets[i] == client_socket) continue;
      write(client_sockets[i], msg, sizeof(msg));
    }
  }

}

int main() {

  // server socket creation
  server_socket = socket(AF_INET, SOCK_STREAM, 0);

  // server address definitoin
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(9002); //ITS OVER 9000 AGAIN!!!!
  server_address.sin_addr.s_addr = INADDR_ANY;


  // bind sockdt to specified IP and port
  printf("Starting server...\n");
  bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));
  listen(server_socket, 5);

  printf("Done\n\nStarted accepting connections.\n");
  while (1) {
    // accepting client
    int client_socket;
    client_socket = accept(server_socket, NULL, NULL);
    client_sockets[client_count++] = client_socket;

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, serve_client, &client_socket);
  }

  pthread_exit(NULL);
  return 0;
}

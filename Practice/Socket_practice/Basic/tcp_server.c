#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

int main() {
  char server_message[256] = "I am groot";

  // server socket creation
  int server_socket;
  server_socket = socket(AF_INET, SOCK_STREAM, 0);

  // server address definitoin
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(9011); //ITS OVER 9000 AGAIN!!!!
  server_address.sin_addr.s_addr = INADDR_ANY;

  // bind sockdt to specified IP and port
  bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));
  listen(server_socket, 5);

  // accepting client
  int client_socket;
  client_socket = accept(server_socket, NULL, NULL);

  read(client_socket, &server_message, sizeof(server_message) );
  printf("recieved: %s\n", server_message);

  // sending message
  // send(client_socket, server_message, sizeof(server_message), 0);

  // closing connection
  close(server_socket);

  return 0;
}

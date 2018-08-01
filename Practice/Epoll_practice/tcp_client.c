#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


int main() {

  char server_response[256] = "some dummy";

  // socket creation
  int network_socket;
  network_socket = socket(AF_INET, SOCK_STREAM, 0);

  // address creation for socket (where to connect)
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(9011); //ITS OVER 9000!!!!
  server_address.sin_addr.s_addr = INADDR_ANY;

  int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));

  // 0 means successful connection, -1 means failure(error number stored in errno)
  if (connection_status == -1) {
    printf("There was an error making a connection.\n");
    printf("error: %s\n", strerror(errno));
  }
    else
  {
    sleep(3);
    // receiving information and printing
    send(network_socket, &server_response, sizeof(server_response), 0);
    recv(network_socket, &server_response, sizeof(server_response), 0);
    printf("server response: %s\n", server_response);
  }
  // while(1){}
  // closing socket
  close(network_socket);

  return 0;
}

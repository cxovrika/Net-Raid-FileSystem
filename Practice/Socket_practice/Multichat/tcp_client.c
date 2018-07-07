#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

void* listener_thread(void* network_socket_) {
  int network_socket = *((int*)network_socket_);
  char server_response[256];

  while (1) {
    read(network_socket, &server_response, sizeof(server_response));
    printf("%s\n", server_response);
  }
}

void* speaker_thread(void* network_socket_) {
  int network_socket = *((int*)network_socket_);
  char message_to_send[256];

  printf("Enter nickanme: ");
  scanf("%s", message_to_send);
  write(network_socket, message_to_send, sizeof(message_to_send));

  while (1) {
    scanf("%s", message_to_send);
    write(network_socket, message_to_send, sizeof(message_to_send));
  }
}



int main() {

  // socket creation
  int network_socket;
  network_socket = socket(AF_INET, SOCK_STREAM, 0);

  // address creation for socket (where to connect)
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(9002); //ITS OVER 9000!!!!
  server_address.sin_addr.s_addr = INADDR_ANY;

  int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));

  // 0 means successful connection, -1 means failure(error number stored in errno)
  if (connection_status == -1) {
    printf("There was an error making a connection.\n");
  }
    else
  {
    pthread_t listener_thread_id;
    pthread_t speaker_thread_id;

    pthread_create(&listener_thread_id, NULL, listener_thread, &network_socket);
    pthread_create(&speaker_thread_id,  NULL,  speaker_thread, &network_socket);
  }

  pthread_exit(NULL);
}

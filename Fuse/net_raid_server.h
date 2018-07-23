#ifndef NET_RAID_SERVER_H
#define NET_RAID_SERVER_H
#define BACKLOG 10

char client_ip[32];
char path_to_storage[1024];
char message[256];

int server_socket, client_socket, PORT;
struct sockaddr_in server_address;

#endif

CC = gcc

CFLAGS = -lpthread -std=c99

server:
	./tcp_server

client:
	./tcp_client

compile: tcp_server.o tcp_client.o
	$(CC) tcp_server.o -o tcp_server $(CFLAGS)
	$(CC) tcp_client.o -o tcp_client $(CFLAGS)

tcp_server.o: tcp_server.c
	$(CC) -c tcp_server.c $(CFLAGS)

tcp_client.o: tcp_client.c
	$(CC) -c tcp_client.c $(CFLAGS)

clean:
	rm -rf *.o tcp_server tcp_client

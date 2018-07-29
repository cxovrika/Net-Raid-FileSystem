MOUNTPOINT1 = ./mountpoint1

server1:
	./net_raid_server 127.0.0.1 10001 ./serverside1

server2:
	./net_raid_server 127.0.0.1 10002 ./serverside2

hotswap:
	./net_raid_server 127.0.0.1 11111 ./hotswapside


client:
	./net_raid_client ./configurations

compile:
	gcc -std=gnu99 net_raid_server.c md5.c -o net_raid_server
	gcc -std=gnu99 net_raid_client.c md5.c -o net_raid_client
	gcc -std=gnu99 -Wall fusecxR1.c `pkg-config fuse --cflags --libs` -D_FILE_OFFSET_BITS=64 -o fusecxR1

clean:
	rm net_raid_server -f
	rm net_raid_client -f
	rm fusecxR1 -f
	fusermount -u $(MOUNTPOINT1)

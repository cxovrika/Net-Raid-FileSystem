server:
	./net_raid_server 127.0.0.1 10001 /path/to/storage_dir1

client:
	./net_raid_client ./configurations

compile:
	gcc -std=gnu99 net_raid_server.c -o net_raid_server
	gcc -std=gnu99 net_raid_client.c -o net_raid_client
	gcc -std=gnu99 -Wall fusecxR1.c `pkg-config fuse --cflags --libs` -D_FILE_OFFSET_BITS=64 -o fusecxR1

clean:
	rm net_raid_server -f
	rm net_raid_client -f
	rm fusecxR1 -f

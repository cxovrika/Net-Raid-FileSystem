MOUNTPOINT = mountpoint

compile:
	gcc -Wall fusexmp.c `pkg-config fuse --cflags --libs` -D_FILE_OFFSET_BITS=64 -o fusexmp

mount:
	mkdir $(MOUNTPOINT)
		./fusexmp $(MOUNTPOINT) -f

clean:
	fusermount -u $(MOUNTPOINT)
	rm -r $(MOUNTPOINT)
	rm fusexmp

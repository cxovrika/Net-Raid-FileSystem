CC = gcc

run:
	./output

compile: main.o message.o
	$(CC) main.o message.o -o output

main.o: main.c
	$(CC) -c -Wall main.c

message.o: message.c message.h
	$(CC) -c message.c

clean:
	rm -rf *.o output

CC=gcc
CFLAGS=-I.

server: nirl_server.o
	$(CC) -o server nirl_server.o
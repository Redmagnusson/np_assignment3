CC = gcc
CC_FLAGS = -w -g



all: client server



client: client.o
	$(CC) -Wall -o cchat client.o

server: server.o
	$(CC) -Wall -o cserverd server.o


clean:
	rm *.o *.a test cserverd cchat

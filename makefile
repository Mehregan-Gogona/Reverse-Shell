CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGETS = client server

all: $(TARGETS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c

server: server.c
	$(CC) $(CFLAGS) -o server server.c

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
CC = gcc
CFLAGS = -g -Wall
OBJ_CLIENT = raw_socket.o kermit.o queue.o client.o
OBJ_SERVER = raw_socket.o kermit.o queue.o server.o
DEPS = raw_socket.h kermit.h queue.h

all: client server

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

# Cria o executavel client
client: $(OBJ_CLIENT)
	$(CC) -o $@ $^ $(CFLAGS)

# Cria o executavel server
server: $(OBJ_SERVER)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	@rm -f *.o

purge: clean
	@rm server client

# EOF ----------------------------------------------------------
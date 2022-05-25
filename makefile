GCC = gcc
FLAGS = -g -Wall

all: server client test


# Test
test: client server
	$(CC) $(FLAGS) -pthread test.c -o test

# Client 
client: Stack.a stackShellLib.a
	$(CC) $(FLAGS) -pthread client.c Stack.a stackShellLib.a -o client 

# Server

server: server.o TCPServer.a stackShellLib.a Stack.a
	$(CC) $(FLAGS) -pthread -o server server.o TCPServer.a stackShellLib.a Stack.a

server.o: server.c TCPServer.h stackShellLib.h
	$(CC) $(FLAGS) -c server.c -pthread

TCPServer.a: TCPServer.o
	$(AR) -rcs TCPServer.a TCPServer.o

TCPServer.o: TCPServer.c TCPServer.h
	$(CC) $(FLAGS) -c TCPServer.c -pthread


#stackShellLib

stackShellLib.a: stackShellLib.o Stack.o mlock.o 
	$(AR) -rcs stackShellLib.a stackShellLib.o Stack.o mlock.o

stackShellLib.o: stackShellLib.c stackShellLib.h mlock.h
	$(CC) $(FLAGS) -c stackShellLib.c -pthread

# Stack

Stack.a: Stack.o mlock.o
	$(AR) -rcs Stack.a Stack.o mlock.o

Stack.o: Stack.c Stack.h mlock.h
	$(CC) $(FLAGS) -c Stack.c -pthread

# Memory

mlock.a: mlock.o
	$(AR) -rcs mlock.a mlock.o -pthread

mlock.o: mlock.c mlock.h
	$(CC) $(FLAGS) -pthread -c mlock.c

.PHONY: clean all

clean: # Remove any file that might created.
	rm -f *.o *.a *.so *.gch localShell client server test trash.txt

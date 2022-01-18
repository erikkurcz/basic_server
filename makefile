CC=g++
CFLAGS=-I.
DEPS = sock_location.hh
OBJ = client.o server.o

%.o:%.cpp $(DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)

all: $(OBJ)
	$(CC) -o myserver server.o
	$(CC) -o myclient client.o

debug: $(OBJ)
	$(CC) -g -o myserver server.o
	$(CC) -g -o myclient client.o

clean: 
	rm -rf *.o

d: $(OBJ)
	$(CC) -g -o myserver server.o
	$(CC) -g -o myclient client.o

c: 
	rm -rf *.o

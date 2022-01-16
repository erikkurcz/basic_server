CC=g++
CFLAGS=-I.
DEPS = sock_location.hh
OBJ = client.o server.o

%.o:%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)
	$(CC) -o myserver server.o
	$(CC) -o myclient client.o

clean: 
	rm -rf *.o

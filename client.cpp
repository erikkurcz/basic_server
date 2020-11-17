// A client to connect and read from the same server code I just wrote

#include <iostream>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "sock_location.hh"


int main(int argc, char* argv[]){

    // Basics
    int BUFFER_SIZE = 1024;
    
    // Buffer and related reading and writing
    char buf[BUFFER_SIZE];
    int read_ct(0);
    int written_ct(0);

    // Connection related
    int mysock(-2);
    struct sockaddr_un their_address;
    socklen_t their_address_size;

    // Open up a connection 
    mysock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (mysock == -1){
        std::cerr << "Socket failed to open, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }
    
    // Make necessary structs
    // Clear first then fill
    memset(&their_address, 0, sizeof(struct sockaddr));
    their_address.sun_family = AF_UNIX;
    strncpy(their_address.sun_path, SOCK_PATH, sizeof(their_address.sun_path)-1);

    if (connect(mysock, (struct sockaddr *)&their_address, sizeof(struct sockaddr)) == -1){
        std::cerr << "Failed to connect to socket, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }

    // Read off the connection foreverrrr
    while (read_ct = read(mysock, &buf, BUFFER_SIZE)){

        if (read_ct == -1){
            std::cerr << "Failed to read from socket, errno: " << errno << ": " << strerror(errno) << std::endl;
            return -1;
        }
        
        // Write it to cmdline
        while (read_ct > 0){ 
            written_ct = write(STDOUT_FILENO, &buf, read_ct);
            if (written_ct == -1){
                std::cerr << "Failed to write to their sock, errno: " << errno << ": " << strerror(errno) << std::endl;
                return -1;
            } else {
                // So we keep going if we write fewer than the read message
                read_ct = read_ct - written_ct;
            }

        }

    }
    
    return 0;
}

// A simple server to send data across the network

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
    int CONNECTION_BACKLOG = 10;
    
    // Buffer and related reading and writing
    char buf[BUFFER_SIZE];
    int read_ct(0);
    int written_ct(0);

    // Connection related
    int mysock(-2);
    int theirsock(-2);
    struct sockaddr_un my_address;
    struct sockaddr_un their_address;
    socklen_t their_address_size;

    // Remove the sock path before we begin just in case
    if (system("rm -f SOCKPATH") == -1){
        std::cerr << "Failed to remove SOCK_PATH: `" << SOCK_PATH << "`, errno: " << errno << ":" << strerror(errno) << std::endl;
        return -1;
    }

    // Open up a connection 
    mysock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (mysock == -1){
        std::cerr << "Socket failed to open, errno: " << errno << ":" << strerror(errno) << std::endl;
        return -1;
    }
    
    // Bind to it
   
    // Make necessary structs
    // Clear first then fill
    memset(&my_address, 0, sizeof(struct sockaddr));
    my_address.sun_family = AF_UNIX;
    strncpy(my_address.sun_path, SOCK_PATH, sizeof(my_address.sun_path)-1);

    if (bind(mysock, (struct sockaddr *)&my_address, sizeof(struct sockaddr)) == -1){
        std::cerr << "Failed to bind to socket, errno: " << errno << ":" << strerror(errno) << std::endl;
        return -1;
    }

    // Finally listen then we will be able to connect

    if (listen(mysock, CONNECTION_BACKLOG)){
        std::cerr << "Failed to listen, errno: " << errno << ":" << strerror(errno) << std::endl;
        return -1;
    }
    
    // Wait for connection
    std::cout << "Waiting for connection before accepting data to send..." << std::endl;
    their_address_size = sizeof(struct sockaddr);
    theirsock = accept(mysock, (struct sockaddr *)&their_address, &their_address_size);
    if (theirsock == -1){
        std::cerr << "Failed to accept their sock, errno: " << errno << ":" << strerror(errno) << std::endl;
        return -1;
    } else {
        std::cout << "Master has given Dobby a sock!\n" 
                  << "Connected! Begin writing." << std::endl;
    }

    // Read off command line until Ctrl+C
    errno = 0;
    while (read_ct = read(STDIN_FILENO, &buf, BUFFER_SIZE)){
        
        // Send ittttt
        while (read_ct > 0){ 

            written_ct = write(theirsock, &buf, read_ct);
            if (written_ct == -1){
                std::cerr << "Failed to write to their sock, errno: " << errno << ":" << strerror(errno) << std::endl;
                return -1;
            } else {
                // So we keep going if we write fewer than the read message
                read_ct = read_ct - written_ct;
            }

        }

    }

    if (errno != 0){
        std::cerr << "Failed to read from command line, errno: " << errno << ":" << strerror(errno) << std::endl;
        return -1;
    }
    
    // Remove the sock path
    if (remove(SOCK_PATH) == -1){
        std::cerr << "Failed to remove SOCK_PATH: `" << SOCK_PATH << "`, errno: " << errno << ":" << strerror(errno) << std::endl;
        return -1;
    }

    return 0;
}

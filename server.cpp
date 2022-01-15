// A simple server to send data across the network

#include <iostream>
#include <string>

#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "sock_location.hh"


void usage(void){
    printf("mysever N\n\nWhere N is number of seconds to sleep between writing chars to client\n");
}

int main(int argc, char* argv[]){

    // Get sleep time between chars off cmdline
    if (argc > 2){
        usage();
        exit(1);
    }

    int sleep_seconds(0);
    if (argc == 2){
        sleep_seconds = atoi(argv[1]);
    }

    // Basic
    int BUFFER_SIZE = 1024;
    int CONNECTION_BACKLOG = 10;

    // Buffer and related reading and writing
    char buf[BUFFER_SIZE];
    int read_ct(0);
    int written_ct(0);

    // Connection related
    int mysock(-2);
    int theirsock(-2);
    struct sockaddr_in my_address;
    struct in_addr my_inet_address;
    struct sockaddr_in their_address;
    socklen_t their_address_size;

    // Remove the sock path before we begin just in case
    // Don't really care if it fails
    remove(SOCK_PATH);

//    if (remove(SOCK_PATH) == -1){
//        std::cerr << "Failed to remove SOCK_PATH: `" << SOCK_PATH << "`, errno: " << errno << ": " << strerror(errno) << std::endl;
//        return -1;
//    }

    // Open up a connection
    mysock = socket(AF_INET, SOCK_STREAM, 0);
    if (mysock == -1){
        std::cerr << "Socket failed to open, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }

    // Bind to it

    // Make necessary structs
    // Clear first then fill
    memset(&my_address, 0, sizeof(struct sockaddr_in));
    my_address.sin_family = AF_INET;
    my_address.sin_port = htons(SERVER_PORT_NUMBER);

    my_inet_address.s_addr = INADDR_LOOPBACK;
    my_address.sin_addr = my_inet_address;

    if (bind(mysock, (struct sockaddr_in *)&my_address, sizeof(struct sockaddr_in)) == -1){
        std::cerr << "Failed to bind to socket, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }

    // Finally listen then we will be able to connect

    if (listen(mysock, CONNECTION_BACKLOG)){
        std::cerr << "Failed to listen, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }

    // Wait for connection
    std::cout << "Waiting for connection before accepting data to send..." << std::endl;
    their_address_size = sizeof(struct sockaddr_in);
    theirsock = accept(mysock, (struct sockaddr_in *)&their_address, &their_address_size);
    if (theirsock == -1){
        std::cerr << "Failed to accept their sock, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    } else {
        std::cout << "Master has given Dobby a sock!\n"
                  << "Connected! Begin writing." << std::endl;
    }

    // Read off command line until Ctrl+C
    errno = 0;
    int to_write(0);
    while (read_ct = read(STDIN_FILENO, &buf, BUFFER_SIZE)){

        // Send ittttt
        while (read_ct > 0){

            // sly fox sleeps between each read
            if (sleep_seconds > 0){
                sleep(sleep_seconds);
            }

            written_ct = write(theirsock, &buf, read_ct);

            if (written_ct == -1){
                std::cerr << "Failed to write to their sock, errno: " << errno << ": " << strerror(errno) << std::endl;
                return -1;
            } else {
                // So we keep going if we write fewer than the read message
                read_ct = read_ct - written_ct;
            }

        }

    }

    if (errno != 0){
        std::cerr << "Failed to read from command line, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }

    // Remove the sock path
    if (remove(SOCK_PATH) == -1){
        std::cerr << "Failed to remove SOCK_PATH: `" << SOCK_PATH << "`, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }

    return 0;
}

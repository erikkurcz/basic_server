// A client to connect and read from the same server code I just wrote

#include <iostream>
#include <string>

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "sock_utils.hh"

void usage(){
    printf("Usage: ./myclient PORT\n\nWhere\n\tPORT is the port number to connect on\n");
}


int main(int argc, char* argv[]){

    if (argc != 2){
        usage();
        return 1;
    }

    const int PORT_NUMBER = atoi(argv[1]);

    // Basics
    int BUFFER_SIZE = 1024;
    
    // Buffer and related reading and writing
    char buf[BUFFER_SIZE];
    char udata_buf[BUFFER_SIZE];
    int read_ct(0);
    int written_ct(0);

    // Connection related
    int mysock(-2);
    struct sockaddr_in their_address;
    struct in_addr their_inet_address;
    socklen_t their_address_size;
    
    // for working with user data
    int udata_read_ct = 0;
    int write_to_server_ct = 0;
    int hasdata_rc = 0;


    // Open up a connection 
    mysock = socket(AF_INET, SOCK_STREAM, 0);
    if (mysock == -1){
        std::cerr << "Socket failed to open, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }
    
    // Make necessary structs
    // Clear first then fill
    memset(&their_address, 0, sizeof(struct sockaddr));
    their_address.sin_family = AF_INET;
    their_address.sin_port = htons(PORT_NUMBER);
    their_inet_address.s_addr = htonl(INADDR_LOOPBACK);
    their_address.sin_addr = their_inet_address;

    if (connect(mysock,(struct sockaddr*)&their_address, sizeof(struct sockaddr)) == -1){
        std::cerr << "Failed to connect to socket, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }

    // Read off the connection foreverrrr
    while (read_ct = recv(mysock, &buf, BUFFER_SIZE, 0)){

        if (read_ct == -1){
            std::cerr << "Failed to read from socket, errno: " << errno << ": " << strerror(errno) << std::endl;
            return -1;
        }

        // Write it to cmdline
        while (read_ct > 0){ 
            if (memcmp(&buf, &MAXCONNBUF, MAXCONNBUF_SIZE) == 0){
                std::cerr << "Server at max connections, retry later" << std::endl;
                return 1;
            }
            if (memcmp(&buf, &PINGBUF, PINGBUF_SIZE) == 0){
                break; 
            }
            written_ct = write(STDOUT_FILENO, &buf, read_ct);
            if (written_ct == -1){
                std::cerr << "Failed to write to their sock, errno: " << errno << ": " << strerror(errno) << std::endl;
                return -1;
            } else {
                // So we keep going if we write fewer than the read message
                read_ct = read_ct - written_ct;
            }
        }

        // If user has given data on STDIN to write to server
        // then let's get it and write it over

        udata_read_ct = 0;
        write_to_server_ct = 0;
        hasdata_rc = has_data(STDIN_FILENO);

        if (hasdata_rc == -1){
            std::cerr << "Error while checking if has_data on fd(" << STDIN_FILENO << ")" << std::endl;
            return -1; 
        }

        if (hasdata_rc != 0) {
            udata_read_ct = read(STDIN_FILENO, &udata_buf, BUFFER_SIZE);
            if (udata_read_ct == -1) {
                std::cerr << "Failed to read from STDIN_FILENO into &udata_buf, errno: " << errno << ": " << strerror(errno) << std::endl;
                return -1;
            }
            
            // loop to write all of it
            while (udata_read_ct > 0){
                write_to_server_ct = write(mysock, &udata_buf, udata_read_ct);
                if (write_to_server_ct == -1){
                    std::cerr << "Failed to write user data to their sock, errno: " << errno << ": " << strerror(errno) << std::endl;
                    return -1;
                } else {
                    udata_read_ct = udata_read_ct - write_to_server_ct;
                }
            }
        } // if block for reading from stdin

    } // overall read-off-socket loop
    return 0;
} // main

// A simple server to send data across the network

#include <iostream>
#include <string>

#include <netinet/ip.h>
#include <signal.h>
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

void handle_sigpipe(int signo){
    std::cout << "SIGPIPE received, likely client connection dropped.\n\nOh no! Anyways..." << std::endl;
}

int main(int argc, char* argv[]){

    // Get sleep time between chars off cmdline
    if (argc > 2){
        usage();
        exit(1);
    }

    // set signal handler for SIGPIPE only
    struct sigaction sa;;
    sa.sa_handler = &handle_sigpipe;
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) != 0){ 
        std::cerr << "Failed to set signal handler, errno: " << errno << strerror(errno) << std::endl;
        return -1;
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
    char readbuf[BUFFER_SIZE];
    int read_ct(0);
    int written_ct(0);

    // Connection related
    int mysock(-2);
    int theirsock(-2);
    int sock_count(0); 
    int connection_arr[CONNECTION_BACKLOG];
    memset(connection_arr, 0, CONNECTION_BACKLOG);
    struct sockaddr_in my_address;
    struct in_addr my_inet_address;

    struct sockaddr_in their_address;
    socklen_t their_address_size;

    // Open up a connection
    mysock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (mysock == -1){
        std::cerr << "Socket failed to open, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }

    // Bind to it

    // Make necessary structs
    // Clear first then fill
    memset(&my_address, 0, sizeof(struct sockaddr));
    my_address.sin_family = AF_INET;
    my_address.sin_port = htons(PORT_NUMBER);
    
    // set INADDR_LOOPBACK for in_addr struct, then assign struct to sockaddr_in struct
    my_inet_address.s_addr = htonl(INADDR_LOOPBACK);
    my_address.sin_addr = my_inet_address;
    
    // finally bind
    if (bind(mysock, (struct sockaddr*)&my_address, sizeof(struct sockaddr)) == -1){
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
    their_address_size = sizeof(struct sockaddr);
    
    // keep looking for new connections
    while (true){


        // Read off command line until Ctrl+C
        errno = 0;
        int to_write(0);
        int local_read_ct(0);
        int read_rc(0);
        int idx(0);
        while (read_ct = read(STDIN_FILENO, &buf, BUFFER_SIZE)){

            // accept a new connection
            connection_arr[sock_count] = accept(mysock, (struct sockaddr *)&their_address, &their_address_size);

            if (connection_arr[sock_count] == -1){
                // failed to connect
                
                // EAGAIN or EWOULDBLOCK means no waiting connection, we can ignore
                // But if it's either of those reasons, that's a problem
                if (errno != EAGAIN && errno != EWOULDBLOCK){
                    std::cerr << "Failed to accept their sock, errno: " << errno << ": " << strerror(errno) << std::endl;
                    return -1;
                }
            } else {
                // got a valid connection
                std::cout << "Master has given Dobby a sock!\n"
                          << "Connected! Begin writing." << std::endl;
                sock_count += 1;
            }
            
            idx = 0;
            while (idx < sock_count){

                if (connection_arr[idx] > 0) {
                    // we have an actual fd here that we can write to, so let's do so
                    local_read_ct = read_ct;
                    // Send ittttt
                    while (local_read_ct > 0){

                        // sly fox sleeps between each read
                        if (sleep_seconds > 0){
                            sleep(sleep_seconds);
                        }
                        std::cout << "verifying connection is alive @ idx: " << idx << " to fd=" << connection_arr[idx] << std::endl;
                        read_rc = read(connection_arr[idx], &readbuf, 0);
                        if (read_rc == -1){
                            std::cout << "connection @ idx " << idx << " to fd=" << connection_arr[idx] << " is invalid: errno: " << errno << std::endl;
                            return -1;
                        }

                        if (readbuf[0] == 'x'){ 
                            // client disconnected, set to zero
                            std::cout << "client at connection @ idx: " << idx << " has disconnected" << std::endl;
                            connection_arr[idx] = 0;
                            sock_count--;

                        } else {

                            // client still there, write to it

                            std::cout << "writing to connection @ idx: " << idx << " to fd=" << connection_arr[idx] << std::endl;
                            errno = 0;
                            written_ct = send(connection_arr[idx], &buf, local_read_ct, MSG_NOSIGNAL);
                            // write might not return -1 in event of a socket connection lost/closed by the client side, but errno would be set to EAGAIN or EWOULDBLOCK in event it's a non-blocking socket but the write would block
                            // check for that here and alter the idx position if this is the case
                            if (errno == EAGAIN || errno == EWOULDBLOCK){
                                std::cout << "writing to connection @ idx: " << idx << " to fd=" << connection_arr[idx] << " failed to write properly, removing from list" << std::endl;
                                connection_arr[idx] = 0;
                                sock_count--;
                                break;
                            }

                            if (errno == EPIPE){
                                std::cout << "send() to connection @ idx: " << idx << " to fd=" << connection_arr[idx] << " failed to send, received EPIPE, likely dropped connection and am removing from list" << std::endl;
                                connection_arr[idx] = 0;
                                sock_count--;
                                written_ct = 0;
                                idx--;
                                break;
                            }

                            if (written_ct == -1){
                                std::cerr << "Failed to write to their sock, errno: " << errno << ": " << strerror(errno) << ", idx=" << idx << ", fd=" << connection_arr[idx] << std::endl;
                                return -1;
                            } else {
                                // So we keep going if we write fewer than the read message
                                local_read_ct = local_read_ct - written_ct;
                            }
                        }
                    }
                }
                idx++;
            }
        }

        if (errno != 0){
            std::cerr << "Failed to read from command line, errno: " << errno << ": " << strerror(errno) << std::endl;
            return -1;
        }

    } // end of while(true) loop

    return 0;
}

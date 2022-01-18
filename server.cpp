// A simple server to send data across the network

#include <algorithm>
#include <iostream>
#include <string>
#include <list>
#include <vector>

#include <netinet/ip.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "sock_utils.hh"


void usage(void){
    printf("myserver PORT MAX_CONNECTIONS DEBUG\n\nWhere\n\tPORT is port number to use\n\tMAX_CONNECTIONS is the max number of connections this server will accept.\n\tDEBUG is any character to indicate you'd like extra debug logging\n");
}

void handle_sigpipe(int signo){
    std::cout << "SIGPIPE received, likely client connection dropped.\n\nOh no! Anyways..." << std::endl;
}

int main(int argc, char* argv[]){

    if (argc != 3 && argc != 4){
        usage();
        exit(1);
    }

    const int PORT_NUMBER = atoi(argv[1]);
    const int MAX_CONNECTIONS = atoi(argv[2]);
    bool debug = false;
    if (argc == 4){
        debug = true;
    }

    // set signal handler for SIGPIPE only
    struct sigaction sa;
    sa.sa_handler = &handle_sigpipe;
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) != 0){ 
        std::cerr << "Failed to set signal handler, errno: " << errno << strerror(errno) << std::endl;
        return -1;
    }

    // Basic

    // Buffer and related reading and writing
    char buf[BUFFER_SIZE];
    char readbuf[BUFFER_SIZE];
    int read_ct(0);
    int written_ct(0);

    // Connection related
    int mysock(-2);
    int theirsock(-2);
    std::list<int> connection_arr;
    struct sockaddr_in my_address;
    struct in_addr my_inet_address;

    struct sockaddr_in their_address;
    socklen_t their_address_size;

    // Open up a socket
    mysock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (mysock == -1){
        std::cerr << "Socket failed to open, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }

    // set SO_REUSEADDR to allow faster turnaround in testing
    int truthy_int(1);
    if (setsockopt(mysock, SOL_SOCKET, SO_REUSEADDR, &truthy_int, sizeof(int)) != 0){
        std::cerr << "failed to set SO_REUSEADDR, errno: " << errno << ": " << strerror(errno) << std::endl;
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

    if (listen(mysock, 10)){
        std::cerr << "Failed to listen, errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }

    std::cout << "Ready for input:" << std::endl;
    
    // keep looking for new connections
    int newfd;
    while (true){

        // Read off command line until Ctrl+C
        errno = 0;
        int to_write(0);
        int local_read_ct(0);
        int read_rc(0);
        bool bad_connections = true;
        while (read_ct = read(STDIN_FILENO, &buf, BUFFER_SIZE)){

            // Check if we have any dead connections we can remove before we accept new ones
            // always assume there are bad_connections, set it to true
            bad_connections = true;
            while (bad_connections){
                for (std::list<int>::iterator conn_iter = connection_arr.begin();
                        conn_iter != connection_arr.end();
                        conn_iter++){
                    // ping twice because that's what it takes apparently

                    bool is_bad = false;
                    for (int attempt = 1; attempt <= 2; attempt++){
                        written_ct = send(*conn_iter, &PINGBUF, PINGBUF_SIZE, MSG_NOSIGNAL);

                        if (written_ct == -1){
                            // something's wrong 
                            if (debug) std::cout << "pinging conn fd=" << *conn_iter << ", attempt=" << attempt << " - failure" <<  std::endl;

                            if (errno == EPIPE || errno == ENOTSOCK || errno == EBADF){
                                // when we have encountered a fd that is no longer a socket, either dropped or never was
                                if (debug) std::cout << "Removing bad connection: " << *conn_iter << std::endl;
                                close(*conn_iter);
                                connection_arr.erase(conn_iter);
                                is_bad = true;
                                if (debug) std::cout << "connection_arr.size()=" << connection_arr.size() << std::endl;
                                break; // connection_arr may have reallocated, retry the loop 
                            } else {
                                std::cerr << "Failed to write to their sock, errno: " << errno << ": " << strerror(errno) << " fd=" << *conn_iter << std::endl;
                                return -1;
                            }
                        }
                        if (debug) std::cout << "pinging conn fd=" << *conn_iter << ", attempt=" << attempt << " - success" <<  std::endl;
                    }
                    if (is_bad){
                        break;
                    }

                }
                bad_connections = false;
            }
            
            // **** LOOK FOR NEW CONNECTIONS **** 
            // loop to accept all new connections
            while (newfd = accept(mysock, (struct sockaddr *)&their_address, &their_address_size)){
                if (newfd == -1){
                    // failed to accept, let's see why
                    if (errno == EAGAIN || errno == EWOULDBLOCK){
                        // EAGAIN or EWOULDBLOCK means no waiting connection, we can ignore
                        if (debug) std::cout << "No connections waiting" << std::endl;
                        break;
                    } else {
                        // actual issue at hand, not just no connection available
                        std::cerr << "Failed to accept their sock, errno: " << errno << ": " << strerror(errno) << std::endl;
                        return -1;
                    }
                }
                if (debug) std::cout << "New connection received" << std::endl;
                if (debug) std::cout << "connection_arr.size()=" << connection_arr.size() << std::endl;
                if (connection_arr.size() >= MAX_CONNECTIONS){
                    if (debug) std::cout << "Not adding connection, MAX_CONNECTIONS full" << std::endl;
                    written_ct = send(newfd, &MAXCONNBUF, MAXCONNBUF_SIZE, MSG_NOSIGNAL);
                    close(newfd);
                    break;
                }
                // accepted new connection
                connection_arr.push_back(newfd);
            }
            // **** END OF LOOK FOR NEW CONNECTIONS **** 

            // **** SEND CMD LINE DATA TO CONNECTIONS **** 
            // for loop for sending what we ready off command line
            for (std::list<int>::iterator conn_iter = connection_arr.begin();
                    conn_iter != connection_arr.end();
                    conn_iter++){

                local_read_ct = read_ct;
                // Send ittttt
                while (local_read_ct > 0){

                    // write to connection
                    if (debug) std::cout << "send to fd=" << *conn_iter << std::endl;
                    errno = 0;
                    written_ct = send(*conn_iter, &buf, local_read_ct, MSG_NOSIGNAL);

                    if (written_ct == -1){
                        // handle errnos

                        if (errno == EPIPE || errno == ENOTSOCK || errno == EBADF){
                            // when we have encountered a fd that is no longer a socket, either dropped or never was
                            written_ct = local_read_ct;
                            close(*conn_iter);
                            break;
                        } else {
                            std::cerr << "Failed to write to their sock, errno: " << errno << ": " << strerror(errno) << " fd=" << *conn_iter << std::endl;
                            return -1;
                        }

                    } else {
                        // we wrote just fine, decrement in case we didn't write enough
                        // So we keep going if we write fewer than the read message
                        local_read_ct = local_read_ct - written_ct;
                    }

                }

            } // end for loop

        } // **** END OF SEND CMD LINE DATA TO CONNECTIONS **** 

        if (errno != 0){
            std::cerr << "Failed to read from command line, errno: " << errno << ": " << strerror(errno) << std::endl;
            return -1;
        }

    } // end of while(true) loop

    return 0;
}

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

#include "sock_location.hh"


void usage(void){
    printf("myserver PORT MAX_CONNECTIONS\n\nWhere\n\tPORT is port number to use\n\tMAX_CONNECTIONS is the max number of connections this server will accept.\n");
}

void handle_sigpipe(int signo){
    std::cout << "SIGPIPE received, likely client connection dropped.\n\nOh no! Anyways..." << std::endl;
}

int main(int argc, char* argv[]){

    // Get sleep time between chars off cmdline
    if (argc != 3){
        usage();
        exit(1);
    }

    const int PORT_NUMBER = atoi(argv[1]);
    const int MAX_CONNECTIONS = atoi(argv[2]);

    // set signal handler for SIGPIPE only
    struct sigaction sa;
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

    // Buffer and related reading and writing
    char buf[BUFFER_SIZE];
    char readbuf[BUFFER_SIZE];
    char errmsgbuf[MAX_CONNECTIONS_RETRY_FLAG_SIZE];
    errmsgbuf[0] = MAX_CONNECTIONS_RETRY_FLAG;
    int read_ct(0);
    int written_ct(0);

    // Connection related
    int mysock(-2);
    int theirsock(-2);
    std::list<int> connection_arr;
    std::vector<int> conns_to_erase;
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

    if (listen(mysock, 100)){
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
            // always assume true
            bad_connections = true;
            while (bad_connections){
                for (std::list<int>::iterator conn_iter = connection_arr.begin();
                        conn_iter != connection_arr.end();
                        conn_iter++){
                    if (recv(*conn_iter, &readbuf, 1, MSG_DONTWAIT) == -1){
                        if (errno == EPIPE || errno == ENOTSOCK || errno == EBADF){
                            std::cout << "Removing bad connection: " << *conn_iter << std::endl;
                            connection_arr.erase(conn_iter);
                            bad_connections = true;
                            break;
                        }
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
                        std::cout << "No connections waiting" << std::endl;
                        break;
                    } else {
                        // actual issue at hand, not just no connection available
                        std::cerr << "Failed to accept their sock, errno: " << errno << ": " << strerror(errno) << std::endl;
                        return -1;
                    }
                }
                std::cout << "New connection received" << std::endl;
                if (connection_arr.size() >= MAX_CONNECTIONS){
                    std::cout << "Not adding connection, MAX_CONNECTIONS full" << std::endl;
                    written_ct = send(newfd, &errmsgbuf, MAX_CONNECTIONS_RETRY_FLAG_SIZE, MSG_NOSIGNAL);
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

                    // sly fox sleeps between each read
                    if (sleep_seconds > 0){
                        sleep(sleep_seconds);
                    }
                    
                    // write to connection
                    std::cout << "send to fd=" << *conn_iter << std::endl;
                    errno = 0;
                    written_ct = send(*conn_iter, &buf, local_read_ct, MSG_NOSIGNAL);

                    if (written_ct == -1){
                        // handle errnos

                        if (errno == EPIPE || errno == ENOTSOCK || errno == EBADF){
                            // when we have encountered a fd that is no longer a socket, either dropped or never was
                            written_ct = local_read_ct;
                            conns_to_erase.push_back(*conn_iter);
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

            // if we need to erase, do so here
            while (conns_to_erase.size() > 0){
                sort(conns_to_erase.begin(), conns_to_erase.end());
                for (std::list<int>::iterator iter = connection_arr.begin();
                        iter != connection_arr.end();
                        iter++){
                    if (*iter == conns_to_erase.front()){
                        connection_arr.erase(iter);
                        conns_to_erase.erase(conns_to_erase.begin());
                        // connection_arr iter is now invalid, break out
                        break;
                    }
                }
            }
            // **** END OF SEND CMD LINE DATA TO CONNECTIONS **** 
        }

        if (errno != 0){
            std::cerr << "Failed to read from command line, errno: " << errno << ": " << strerror(errno) << std::endl;
            return -1;
        }

    } // end of while(true) loop

    return 0;
}

// Sock path to be shared by client and server
#ifndef SOCK_UTILS_H
#define SOCK_UTILS_H

//const char* SOCK_PATH = "/tmp/TMP_SOCK";
const int BUFFER_SIZE = 1024;

const int MAXCONNBUF_SIZE = 7; 
const char MAXCONNBUF[MAXCONNBUF_SIZE] = { 'M','A','X','C','O','N','N' };

const int PINGBUF_SIZE = 4;
const char PINGBUF[PINGBUF_SIZE] = { 'P','I','N','G' };

int has_data(int fd, bool debug=false){
    // Performs a select to determine if this fd can be used for a read immediately
    // 0 if no, 1 if yes, -1 if error
    int num_fds(fd+1);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0; 

    int timeout = 0;

    fd_set read_watch_fds;
    FD_ZERO(&read_watch_fds);
    FD_SET(fd, &read_watch_fds);

    int rc = select(num_fds, &read_watch_fds, NULL, NULL, &tv);
    if (rc == -1){
        std::cerr << "Error while checking select() on fd " << fd << ", errno: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }
    if (debug) std::cout << "has_data: fd=" << fd << ", rc=" << rc << std::endl;
    return rc; // 1 indicates data ready, 0 indicates no data
}

#endif 

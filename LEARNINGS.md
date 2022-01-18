* added working over AF_INET with one connection
* added accepting multiple connections
* learned about listen()'s connection backlog and how it's distinct from an app-imposed maximum number of connections
* added being able to lose a connection
  * learned about SIGPIPE killing the process when attempting to write() to a now-dead fd that has lost connection (rc=141 ugh)
  * implemented block on SIGPIPE for that reason, later changed socket to not use signals and instead put that info into the errno where it's easier to recover from
  * learned about checking errno for ENOTSOCK and EBADF in event that's given
  * learned about using SOCK_NONBLOCK and handling EAGAIN or EWOULDBLOCK for checking if there are any connections queued
  * learned about using std::list to hold connection fds so we can write to each instead of std::vector, which would potentially clobber the memory on a resize
* learned pinging twice is really the only way to make sure a connection is still valid and can be written to (though TCP_KEEPALIVE might do the same)
* learned how to safely remove a connection
* did a poor job of debugging but can see where gdb should've been used to improve speed of testing :)
* learned all about socket, bind, listen, accept, write, connect, and read
* later learned why you'd use send() and recv() in lieu of write() and read() (so you can use flags such as MSG_NOSIGNAL so you can avoid using a signal handler and ignoring SIGPIPE, instead just checking errno after a write())
* learned all about get/setsockopt and SO_REUSEADDR (even though seems like the listen() call trips over it anyway, good to know)
* learned about strcmp, strncmp, and memcmp for client usage
* learned about sending non-printable characters for pings and unable-to-connect messages so user of server can't simply send these and kill an incoming connection
* learned about struct sockaddr and how the derivative forms of it for different protocols use the same struct and cast it in the bind call (it's weird, but it works)
* learned about one-line if statements for simple checks and debug logging (without actually using a real log manager)
* learned about having ot use htons() and htonl() for sin_port and sin_addr with a sockaddr struct to get things to network order



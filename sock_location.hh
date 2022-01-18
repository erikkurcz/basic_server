// Sock path to be shared by client and server
#ifndef SOCK_LOCATION_H
#define SOCK_LOCATION_H

//const char* SOCK_PATH = "/tmp/TMP_SOCK";
const int BUFFER_SIZE = 1024;

const int MAXCONNBUF_SIZE = 7; 
const char MAXCONNBUF[MAXCONNBUF_SIZE] = { 'M','A','X','C','O','N','N' };

const int PINGBUF_SIZE = 4;
const char PINGBUF[PINGBUF_SIZE] = { 'P','I','N','G' };
#endif 

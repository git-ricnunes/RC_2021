#ifndef MSG_H_
#define MSG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void write_buf(int fd, char* buf);
int write_buf_SIGPIPE(int fd, char* buf);
int read_buf(int fd, char* buf, int bufsize);

#endif
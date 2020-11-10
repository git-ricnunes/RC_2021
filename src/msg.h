#ifndef MSG_H_
#define MSG_H_

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void write_buf(int fd, char* buf);
void write_buf_SIGPIPE(int fd, char* buf);
int read_buf(int fd, char* buf, int bufsize);

#endif
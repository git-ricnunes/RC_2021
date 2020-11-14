#ifndef MSG_H_
#define MSG_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NULL_CHECK 1
#define NULL_IGNORE 0
#define LIM_IGNORE -1

void write_buf(int fd, char* buf, int n);
int write_buf_SIGPIPE(int fd, char* buf, int n);
int read_buf(int fd, char* buf, int n, int n_lim, int check_null);

#endif
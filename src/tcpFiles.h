#ifndef TCP_FILES_H_
#define TCP_FILES_H_

#include "msg.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define SP_CHECK 1
#define DATA_SIZE 1024

char data[DATA_SIZE];

void send_file(int fd, char *fname, int sp);
void recv_file(int fd, FILE * fp, int fsize, char * buffer, int buffer_size);

#endif
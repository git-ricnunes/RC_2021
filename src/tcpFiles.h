#ifndef TCP_FILES_H_
#define TCP_FILES_H_

#include "msg.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void send_file(int fd, FILE * fp, int fsize, char * buffer, int buffer_size);
void recv_file(int fd, char * Fname, int fsize, char * buffer, int buffer_size);

#endif
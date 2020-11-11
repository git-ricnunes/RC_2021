#ifndef TIMER_H_
#define TIMER_H_

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void setTimeoutUDP(int fd, int timeoutDefault);
int checkTimeoutUdp(int sockResult);

#endif
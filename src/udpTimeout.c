#include "udpTimeout.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct timeval tv;

void setTimeoutUDP(int fd, int TIMEOUT_DEFAULT) {
    tv.tv_sec = TIMEOUT_DEFAULT;
    tv.tv_usec = 0;

    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

int checkTimeoutUdp(int sockResult) {
    if (sockResult < 0) {
        printf("Error: Timeout Expired.\n");
        printf("Error code: %d\n", errno);
        return -1;
    }
    return 0;
}
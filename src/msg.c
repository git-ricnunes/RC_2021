#include "msg.h"

#include <stdlib.h>

void write_buf(int fd, char* buf) {
    int n_sum = 0;
    int n_msg = strlen(buf);
    int n_sent;

    while (1) {
        n_sent = write(fd, buf, strlen(buf));

        if (n_sent == -1) { /* Err */
            fprintf(stderr,
                    "Error: failed to send "
                    "%s"
                    " to authentication server\n",
                    buf);
            fprintf(stderr, "Error code: %d\n", errno);
            exit(1);
        }

        n_sum += n_sent;
        if (n_sum == n_msg) /* All bytes written */
            return;
        else /* Still bytes left to write */
            continue;
    }
}

int write_buf_SIGPIPE(int fd, char* buf) {
    int n_sum = 0;
    int n_msg = strlen(buf);
    int n_sent;

    while (1) {
        n_sent = write(fd, buf, strlen(buf));

        if (n_sent == -1) { /* Err */
            if (errno != EPIPE) {
                fprintf(stderr,
                        "Error: failed to send "
                        "%s"
                        " to authentication server\n",
                        buf);
                fprintf(stderr, "Error code: %d\n", errno);
                exit(1);
            } else {
                return n_sent;
            }
        }

        n_sum += n_sent;
        if (n_sum == n_msg) /* All bytes written */
            return 0;
        else /* Still bytes left to write */
            continue;
    }
}

int read_buf(int fd, char* buf, int bufsize) {
    int n_sum = 0;
    int n_rec;

    while (1) {
        n_rec = read(fd, buf, bufsize);

        if (n_rec == -1) { /* Err */
            fprintf(stderr, "Error: failed to read message from authentication server\n");
            fprintf(stderr, "Error code: %d\n", errno);
            exit(1);
        }

        n_sum += n_rec;
        if (buf[n_sum - 1] == '\n') /* All bytes read */
            return n_sum;
        else if (buf[n_sum - 1] == '\0') /* All bytes read */
            return n_sum;
        else /* Still bytes left to read */
            continue;
    }
}
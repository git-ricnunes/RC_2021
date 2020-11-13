#include "msg.h"

/* Ensures all bytes from a given buffer are written to a specified file descriptor and exits gracefully otherwise */
void write_buf(int fd, char* buf) {
    int n_sum = 0;
    int n_msg = strlen(buf);
    int n_sent;

    while (1) {
        n_sent = write(fd, buf, (n_msg - n_sum));

        if (n_sent == -1) { /* Err */
            fprintf(stderr, "Error: failed to send ""%s""\n", buf);
            fprintf(stderr, "Error code: %d\n", errno);
            exit(1);
        }

        printf("write: %s", buf);

        n_sum += n_sent;
        if (n_sum == n_msg) /* All bytes written */
            return;
        else { /* Still bytes left to write */
            buf += n_sent;
            continue;
        }
    }
}

/* write_buf but checks for SIGPIPE instead of always exiting */
int write_buf_SIGPIPE(int fd, char* buf) {
    int n_sum = 0;
    int n_msg = strlen(buf);
    int n_sent;

    while (1) {
        n_sent = write(fd, buf, (n_msg - n_sum));

        if (n_sent == -1) { /* Err */
            if (errno != EPIPE) {
                fprintf(stderr, "Error: failed to send ""%s""\n", buf);
                fprintf(stderr, "Error code: %d\n", errno);
                exit(1);
            } else {
                return n_sent;
            }
        }

        n_sum += n_sent;
        if (n_sum == n_msg) /* All bytes written */
            return 0;
        else { /* Still bytes left to write */
            buf += n_sent;
            continue;
        }
    }
}

/* Ensures a message was completely read and saved into a given buffer from a specified file descriptor, i.e. once the character '\n' is reached */
int read_buf(int fd, char* buf, int bufsize) {
    int n_sum = 0;
    int n_buf = bufsize;
    int n_rec;

    while (1) {
        n_rec = read(fd, buf, (bufsize - n_sum));

        if (n_rec == -1) { /* Err */
            fprintf(stderr, "Error: failed to read message\n");
            fprintf(stderr, "Error code: %d\n", errno);
            exit(1);
        }

        n_sum += n_rec;
        if (buf[n_sum - 1] == '\n') /* All bytes read */
            return n_sum;
        else { /* Still bytes left to read */
            buf += n_rec;
            continue;
        }
    }
}

/* read_buf but instead of reading up until a terminating \n, may also return upon surpassing a specified number of characters */
int read_buf_LIMIT(int fd, char* buf, int bufsize, int n_lim) {
    int n_sum = 0;
    int n_rec;

    while (1) {
        n_rec = read(fd, buf, (bufsize - n_sum));

        if (n_rec == -1) { /* Err */
            fprintf(stderr, "Error: failed to read message\n");
            fprintf(stderr, "Error code: %d\n", errno);
            exit(1);
        }

        n_sum += n_rec;
        if ((buf[n_sum - 1] == '\n') || (n_sum > n_lim)) /* Limit surpassed, enough bytes read */
            return n_sum;
        else { /* Still bytes left to read */
            buf += n_rec;
            continue;
        }
    }
}
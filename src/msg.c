#include "msg.h"

/* Ensures all bytes from a given buffer are written to a specified file descriptor and exits gracefully otherwise */
void write_buf(int fd, char* buf, int n) {
    int n_sum = 0;
    int n_sent;

    while (1) {
        n_sent = write(fd, buf, n - n_sum);

        if (n_sent == -1) { /* Err */
            fprintf(stderr, "Error: failed to send ""%s""\n", buf);
            fprintf(stderr, "Error code: %d\n", errno);
            exit(1);
        }

        n_sum += n_sent;

        if (n_sum < n) { /* Still bytes left to send */
            buf += n_sent;
            continue;
        }
        else /* All bytes sent */
            return;
    }
}

/* write_buf but checks for SIGPIPE and returns n_sent so servers can operate accordingly instead of exiting */
int write_buf_SIGPIPE(int fd, char* buf, int n) {
    int n_sum = 0;
    int n_sent;

    while (1) {
        n_sent = write(fd, buf, n - n_sum);

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

        if (n_sum < n) { /* Still bytes left to send */
            buf += n_sent;
            continue;
        }
        else /* All bytes sent */
            return 0;
    }
}

/* Ensures a message was completely read and saved into a given buffer from a specified file descriptor, i.e. once the character '\n' is reached 
 * n_lim: if provided (!= -1) read_buf may also return upon surpassing n_lim read characters 
 * check_null: if provided (== NULL_CHECK) read_buf may also return upon reading an empty message (NULL) from killing a client socket
 */
int read_buf(int fd, char* buf, int n, int n_lim, int check_null) {
    char* buf_sum = buf;
    int n_sum = 0;
    int n_rec;

    while (1) {
        n_rec = read(fd, buf_sum, n - n_sum);

        if (n_rec == -1) { /* Err */
            fprintf(stderr, "Error: failed to read message\n");
            fprintf(stderr, "Error code: %d\n", errno);
            exit(1);
        }

        n_sum += n_rec;

        if (buf[n_sum - 1] == '\n') /* All bytes read */
            break;
        else if (n_sum > n_lim && n_lim != LIM_IGNORE) /* Limit surpassed, enough bytes read */
            break;
        else if (buf[n_sum - 1] == '\0' && check_null == NULL_CHECK)
            break;
        else { /* Still bytes left to read */
            buf_sum += n_rec;
            continue;
        }
    }
    return n_sum;
}
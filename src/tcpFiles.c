#include "tcpFiles.h"

void send_file(int fd, FILE * fp, int fsize, char * buffer, int buffer_size){
	int n_sum = 0;
	int n_read;
	while(1){
		memset(buffer, 0, buffer_size);
		n_read = fread(buffer, 1, buffer_size, fp);
		if (ferror(fp)){ /* Err */
			fprintf(stderr, "Error reading file\n");
			fprintf(stderr, "Error code: %d\n", errno);
			exit(1);
		}
		if (n_read > 0){
			write_buf(fd, buffer);
			n_sum += n_read;
		}
		if (n_sum == fsize){
			memset(buffer, 0, buffer_size);
			sprintf(buffer, "\n");
			write_buf(fd, buffer);
			return;
		}
	}
}

//add send_file_sigpipe

void recv_file(int fd, FILE * fp, int fsize, char * buffer, int buffer_size){
	int n_sum = 0;
	int n_read;
	int n_wrtn;
	while(1){
		if (strlen(buffer) > 0){ /* Write initial data to file if there is any */
			n_wrtn = fwrite(buffer, 1, strlen(buffer), fp); //fwrite = write? (write all characters read)
			if (n_wrtn == -1) { /* Err */
            	fprintf(stderr, "Error: failed to write data to file\n");
            	fprintf(stderr, "Error code: %d\n", errno);
            	exit(1);
        	}
        	n_sum += n_wrtn;
		}
		if ((fsize == (n_sum - 1)) && (buffer[strlen(buffer) - 1] == '\n'))
			return;
		memset(buffer, 0, buffer_size);
		n_read = read(fd, buffer, buffer_size);
		if (n_read == -1) { /* Err */
            fprintf(stderr, "Error: failed to read data\n");
            fprintf(stderr, "Error code: %d\n", errno);
            exit(1);
        }
	}
}
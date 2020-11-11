#include "tcpFiles.h"

void send_file(int fd, FILE * fp, int fsize, char * buffer, int buffer_size){
	int n_sum = 0;
	int n_read;
	while(1){
		memset(buffer, 0, buffer_size);
		n_read = fread(buffer, 1, buffer_size, fp);
		if (ferror(fp)){
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

void recv_file(int fd, char * Fname, int fsize, char * buffer, int buffer_size){
	
}
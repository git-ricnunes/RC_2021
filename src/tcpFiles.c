#include "tcpFiles.h"

void send_file(int fd, char *fname, int sp){
	int n_sum = 0;
	int n_read;
	int n_sp;
	FILE * fp = fopen(fname, "r");
	if (!fp){
		fprintf(stderr, "Error: failed to open file ""%s""\n", fname);
		fprintf(stderr, "Error code: %d\n", errno);
		exit(1);
	}
	if (fseek(fp, 0L, SEEK_END) == -1){
		fprintf(stderr, "Error: failed to positioning file ""%s"" indicator\n", fname);
		fprintf(stderr, "Error code: %d\n", errno);
		exit(1);
	}
	fsize = ftell(fp);
	if (fsize == -1){
		fprintf(stderr, "Error: failed to read file ""%s"" size\n", fname);
		fprintf(stderr, "Error code: %d\n", errno);
		exit(1);
	}
	else if (fsize >= 10000000000){
		fprintf(stderr, "Error: file ""%s"" size too big\n", fname);
		exit(1);
	}
	memset(data, 0, DATA_SIZE);
	sprintf(data, "%d ", fsize);
	if (sp == SP_CHECK){
		n_sp = write_buf_SIGPIPE(fd, data);
		if (n_sp != 0){
			fclose(fp);
			return;
		}
	}
	else
		write_buf(fd, data);
	fseek(fp, 0L, SEEK_SET);
	while(1){
		memset(data, 0, DATA_SIZE);
		n_read = fread(data, 1, DATA_SIZE, fp);
		if (ferror(fp)){ /* Err */
			fprintf(stderr, "Error reading file\n");
			fprintf(stderr, "Error code: %d\n", errno);
			exit(1);
		}
		if (n_read > 0){
			if (sp == SP_CHECK){
				n_sp = write_buf_SIGPIPE(fd, data);
				if (n_sp != 0){
					fclose(fp);
					return;
				}
			}
			else
				write_buf(fd, data);
			n_sum += n_read;
		}
		if (n_sum == fsize){
			memset(data, 0, DATA_SIZE);
			sprintf(data, "\n");
			if (sp == SP_CHECK)
				write_buf_SIGPIPE(fd, data);
			else
				write_buf(fd, data);
			fclose(fp);
			return;
		}
	}
}

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
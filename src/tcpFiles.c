#include "tcpFiles.h"

void send_file(int fd, char* fname, int sp){
	int n_sum = 0;
	int n_read;

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
	int fsize = ftell(fp);
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
		if (write_buf_SIGPIPE(fd, data, strlen(data)) != 0){
			fclose(fp);
			return;
		}
	}
	else
		write_buf(fd, data, strlen(data));

	if (fseek(fp, 0L, SEEK_SET) == -1){
		fprintf(stderr, "Error: failed to positioning file ""%s"" indicator\n", fname);
		fprintf(stderr, "Error code: %d\n", errno);
		exit(1);
	}

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
				if (write_buf_SIGPIPE(fd, data, n_read) != 0){
					fclose(fp);
					return;
				}
			}
			else
				write_buf(fd, data, n_read);

			n_sum += n_read;
		}
		if (n_sum < fsize)
			continue;
		else {
			memset(data, 0, DATA_SIZE);
			sprintf(data, "\n");
			if (sp == SP_CHECK)
				write_buf_SIGPIPE(fd, data, strlen(data));
			else
				write_buf(fd, data, strlen(data));
			fclose(fp);
			return;
		}
	}
}

void recv_file(int fd, char* fname, int fsize, char* initial_data, int initial_data_size){
	int n_sum = 0;
	int n_read = initial_data_size;
	int n_wrtn;

	FILE * fp = fopen(fname, "a");
	if (!fp){
		fprintf(stderr, "Error: failed to open file ""%s""\n", fname);
		fprintf(stderr, "Error code: %d\n", errno);
		exit(1);
	}

	/* Write initial data */
	if (initial_data_size > fsize)
		n_read = fsize; /* Ignorar '\n' */
	n_wrtn = fwrite(initial_data, 1, n_read, fp);

	while(1){
		if (n_wrtn == -1) { /* Err */
            fprintf(stderr, "Error: failed to write data to file\n");
            fprintf(stderr, "Error code: %d\n", errno);
            exit(1);
        }
        n_sum += n_read;
		if (n_sum >= fsize){
			fclose(fp);
			return;
		}
		memset(data, 0, DATA_SIZE);
		n_read = read(fd, data, DATA_SIZE);
		if (n_read == -1) { /* Err */
            fprintf(stderr, "Error: failed to read data\n");
            fprintf(stderr, "Error code: %d\n", errno);
            exit(1);
        }

        /* Write read data */
        if (n_sum + n_read >= fsize)
        	n_read = fsize - n_sum; /* Ignorar '\n' */
        n_wrtn = fwrite(data, 1, n_read, fp);
	}
}
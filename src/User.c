#include "msg.h"
#include "tcpFiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>

#define DEFAULT_ASIP "127.0.0.1"
#define DEFAULT_ASPORT "58011"
#define DEFAULT_FSIP "127.0.0.1"
#define DEFAULT_FSPORT "59011"

#define IP_SIZE 64
#define PORT_SIZE 16
#define BUFFER_SIZE 128
#define FBUFFER_SIZE 1024
#define MAX_TOKENS 4
#define CODE_SIZE 4
#define UID_SIZE 6
#define PASS_SIZE 9
#define RID_SIZE 5
#define TID_SIZE 5
#define FNAME_SIZE 25
#define FSIZE_SIZE 11
#define RRT_SIZE 18
#define STATUS_SIZE 6
#define LOGGED_IN 1
#define LOGGED_OUT 0
#define AS_FD_SET 1
#define FS_FD_SET 2

int fdAS, fdFS, errcode;
ssize_t n;
socklen_t addrlenAS, addrlenFS;
struct addrinfo hintsAS, *resAS, hintsFS, *resFS;
struct sockaddr_in addrAS, addrFS;

void unexpected_protocol(){
	freeaddrinfo(resAS);
	close(fdAS);
	fprintf(stderr, "ERR\n");
	exit(1);
}

void unexpected_protocol_FS(){
	freeaddrinfo(resFS);
	close(fdFS);
	unexpected_protocol();
}

int main(int argc, char *argv[]){

	char ipAS[IP_SIZE] = DEFAULT_ASIP;
	char portAS[PORT_SIZE] = DEFAULT_ASPORT;
	char ipFS[IP_SIZE] = DEFAULT_FSIP;
	char portFS[PORT_SIZE] = DEFAULT_FSPORT;

	for (int i = 1; i < argc - 1; i++){
		if (argv[i][0] != '-' || strlen(argv[i]) != 2) continue;
		switch (argv[i][1]){
			case 'n':
				if (argv[i+1][0] != '-' || strlen(argv[i+1]) != 2)
					strcpy(ipAS, argv[i+1]);
				break;
			case 'p':
				if (argv[i+1][0] != '-' || strlen(argv[i+1]) != 2)
					strcpy(portAS, argv[i+1]);
				break;
			case 'm':
				if (argv[i+1][0] != '-' || strlen(argv[i+1]) != 2)
					strcpy(ipFS, argv[i+1]);
				break;
			case 'q':
				if (argv[i+1][0] != '-' || strlen(argv[i+1]) != 2)
					strcpy(portFS, argv[i+1]);
				break;
			default:
				fprintf(stderr, "Invalid flag\n");
				fprintf(stderr, "Usage: ./user [-n ASIP] [-p ASport] [-m FSIP] [-q FSPort]\n");
				exit(1);
		}
	}

	fdAS = socket(AF_INET, SOCK_STREAM, 0);
	if (fdAS == -1){
		fprintf(stderr, "Error: unable to create tcp client socket\n");
		fprintf(stderr, "Error code: %d\n", errno);
		exit(1);
	}
		
	memset(&hintsAS, 0, sizeof hintsAS);
	hintsAS.ai_family = AF_INET;
	hintsAS.ai_socktype = SOCK_STREAM;
		
	errcode = getaddrinfo(ipAS, portAS, &hintsAS, &resAS);
	if (errcode != 0){
		fprintf(stderr, "Error: unable to get authentication server address info\n");
		fprintf(stderr, "Error code: %d\n", errno);
		exit(1);
	}

	n = connect(fdAS, resAS->ai_addr, resAS->ai_addrlen);
	if(n == -1){
		fprintf(stderr, "Error: failed to connect to authentication server\n");
		fprintf(stderr, "Error code: %d\n", errno);
		exit(1);
	}

	printf("Connected to Authentication Server ""%s"" in port %s\n", ipAS, portAS);

	FILE * fp;

	char buffer[BUFFER_SIZE];
	char fbuffer[FBUFFER_SIZE];
	char msg[BUFFER_SIZE];
	char code[CODE_SIZE] = "";
	char uid[UID_SIZE] = "";
	char pass[PASS_SIZE] = "";
	char rid[RID_SIZE] = "";
	char tid[TID_SIZE] = "0";
	char fname[FNAME_SIZE] = "";
	char sfsize[FSIZE_SIZE] = "";
	int fsize;
	char rcode[CODE_SIZE] = "";
	char status[STATUS_SIZE] = "";
	int session = LOGGED_OUT;

	srand(time(NULL));

	while(1){

		memset(buffer, 0, BUFFER_SIZE);
		memset(msg, 0, BUFFER_SIZE);
		memset(code, 0, CODE_SIZE);
		memset(fname, 0, FNAME_SIZE);
		memset(fbuffer, 0, FBUFFER_SIZE);
		memset(rcode, 0, CODE_SIZE);
		memset(status, 0, STATUS_SIZE);

		printf("Enter a command: ");
		fgets(buffer, BUFFER_SIZE, stdin);

		int num_tokens = 0;
		char * token_list[MAX_TOKENS];

		char * token = strtok(buffer, " \n");
		while (token != NULL && num_tokens < MAX_TOKENS){
			token_list[num_tokens++] = token;
			token = strtok(NULL, " \n");
		}

		int fd = 0;

		if (!strcmp(token_list[0], "login") && num_tokens == 3){ /* LOG UID pass */
			if (session == LOGGED_IN){
				printf("Already logged in as uid %s\n", uid);
				continue;
			}
			sprintf(code, "LOG");
			if (strlen(token_list[1]) >= UID_SIZE){
				printf("Invalid UID %s: length must not exceed %d characters\n", token_list[1], UID_SIZE - 1);
				continue;
			}
			strcpy(uid, token_list[1]);
			if (strlen(token_list[2]) >= PASS_SIZE){
				printf("Invalid password %s: length must not exceed %d characters\n", token_list[2], PASS_SIZE - 1);
				continue;
			}
			strcpy(pass, token_list[2]);
			sprintf(msg, "%s %s %s\n", code, uid, pass);
			fd = AS_FD_SET;
		}
		else if (!strcmp(token_list[0], "req")){ /* REQ UID RID Fop [Fname] */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				printf("Login command: login UID pass\n");
				continue;
			}
			sprintf(code, "REQ");
			sprintf(rid, "%d%d%d%d", rand()%9, rand()%9, rand()%9, rand()%9);
			if ((!strcmp(token_list[1], "L") || !strcmp(token_list[1], "X")) && num_tokens == 2){
				sprintf(msg, "%s %s %s %s\n", code, uid, rid, token_list[1]);
				fd = AS_FD_SET;
			}
			else if ((!strcmp(token_list[1], "R") || !strcmp(token_list[1], "U") || !strcmp(token_list[1], "D")) && num_tokens == 3){
				if (strlen(token_list[2]) >= FNAME_SIZE){
					printf("Invalid file name %s: length must not exceed %d characters\n", token_list[2], FNAME_SIZE - 1);
					continue;
				}
				strcpy(fname, token_list[2]);
				sprintf(msg, "%s %s %s %s %s\n", code, uid, rid, token_list[1], fname);
				fd = AS_FD_SET;
			}
		}
		else if (!strcmp(token_list[0], "val") && num_tokens == 2){ /* AUT UID RID VC */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				printf("Login command: login UID pass\n");
				continue;
			}
			sprintf(code, "AUT");
			sprintf(msg, "%s %s %s %s\n", code, uid, rid, token_list[1]);
			fd = AS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "list") || !strcmp(token_list[0], "l")) && num_tokens == 1){ /* LST UID TID */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				printf("Login command: login UID pass\n");
				continue;
			}
			sprintf(code, "LST");
			sprintf(msg, "%s %s %s\n", code, uid, tid);
			fd = FS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "retrieve") || !strcmp(token_list[0], "r")) && num_tokens == 2){ /* RTV UID TID Fname */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				printf("Login command: login UID pass\n");
				continue;
			}
			sprintf(code, "RTV");
			if (strlen(token_list[1]) >= FNAME_SIZE){
				printf("Invalid file name %s: length must not exceed %d characters\n", token_list[1], FNAME_SIZE - 1);
				continue;
			}
			strcpy(fname, token_list[1]);
			sprintf(msg, "%s %s %s %s\n", code, uid, tid, fname);
			fd = FS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "upload") || !strcmp(token_list[0], "u")) && num_tokens == 2){ /* UPL UID TID Fname Fsize data */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				printf("Login command: login UID pass\n");
				continue;
			}
			sprintf(code, "UPL");
			if (strlen(token_list[1]) >= FNAME_SIZE){
				printf("Invalid file name %s: length must not exceed %d characters\n", token_list[1], FNAME_SIZE - 1);
				continue;
			}
			strcpy(fname, token_list[1]);
			fp = fopen(fname, "r");
			if (!fp){
				printf("File %s does not exist or cannot be accessed\n", fname);
				continue;
			}
			if (fseek(fp, 0L, SEEK_END) == -1){
				printf("Could not position file %s indicator to determine size\n", fname);
				continue;
			}
			fsize = ftell(fp);
			if (fsize == -1){
				printf("Could not read file %s size\n", fname);
				continue;
			}
			if (fsize >= 10000000000){
				printf("File %s too large (%d GB), should be lower than 10 GB\n", fname, fsize/1000000000);
				continue;
			}
			fclose(fp);
			sprintf(msg, "%s %s %s %s ", code, uid, tid, fname);
			fd = FS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "delete") || !strcmp(token_list[0], "d")) && num_tokens == 2){ /* DEL UID TID Fname */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				printf("Login command: login UID pass\n");
				continue;
			}
			sprintf(code, "DEL");
			if (strlen(token_list[1]) >= FNAME_SIZE){
				printf("Invalid file name %s: length must not exceed %d characters\n", token_list[1], FNAME_SIZE - 1);
				continue;
			}
			strcpy(fname, token_list[1]);
			sprintf(msg, "%s %s %s %s\n", code, uid, tid, fname);
			fd = FS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "remove") || !strcmp(token_list[0], "x")) && num_tokens == 1){ /* REM UID TID */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				printf("Login command: login UID pass\n");
				continue;
			}
			sprintf(code, "REM");
			sprintf(msg, "%s %s %s\n", code, uid, tid);
			fd = FS_FD_SET;
		}
		else if (!strcmp(token_list[0], "exit") && num_tokens == 1){
			freeaddrinfo(resAS);
			close(fdAS);
			exit(0);
		}
		else{
			printf("Invalid command\n");
			continue;
		}

		memset(buffer, 0, BUFFER_SIZE);
		
		if (fd == AS_FD_SET){

			write_buf(fdAS, msg);
		
			n = read_buf(fdAS, buffer, sizeof(buffer));

			sscanf(buffer, "%s %s", rcode, status);

			if (!strcmp(code, "LOG") && !strcmp(rcode, "RLO")){
				if (!strcmp(status, "OK"))
					session = LOGGED_IN;
			}
			else if (!strcmp(code, "AUT") && !strcmp(rcode, "RAU")){
				if (strcmp(status, "0"))
					strcpy(tid, status);
			}
			else if (strcmp(code, "REQ") || strcmp(rcode, "RRQ"))
				unexpected_protocol();

			write(1, "echo: ", 6); write(1, buffer, n);
		}
		else if (fd == FS_FD_SET){

			fdFS = socket(AF_INET, SOCK_STREAM, 0);
			if (fdFS == -1){
				fprintf(stderr, "Error: unable to create tcp client socket\n");
				fprintf(stderr, "Error code: %d\n", errno);
				exit(1);
			}
		
			memset(&hintsFS, 0, sizeof hintsFS);
			hintsFS.ai_family = AF_INET;
			hintsFS.ai_socktype = SOCK_STREAM;
		
			errcode = getaddrinfo(ipFS, portFS, &hintsFS, &resFS);
			if (errcode != 0){
				fprintf(stderr, "Error: unable to get authentication server address info\n");
				fprintf(stderr, "Error code: %d\n", errno);
				exit(1);
			}

			n = connect(fdFS, resFS->ai_addr, resFS->ai_addrlen);
			if(n == -1){
				fprintf(stderr, "Error: failed to connect to file server\n");
				fprintf(stderr, "Error code: %d\n", errno);
				exit(1);
			}

			printf("Connected to File Server ""%s"" in port %s\n", ipFS, portFS);

			write_buf(fdFS, msg);
			if (!strcmp(code, "UPL"))
				send_file(fdFS, fname, SP_IGNORE);

			if (!strcmp(code, "LST")){
				n = read_buf(fdFS, fbuffer, FBUFFER_SIZE); //read_buf_LIMIT?
				write(1, "echo: ", 6); write(1, fbuffer, n);
				//strtok to show one file per line
				//working but still needs to check for unexpected protocol messages
			}
			else if (!strcmp(code, "RTV")){
				n = read_buf_LIMIT(fdFS, fbuffer, FBUFFER_SIZE, RRT_SIZE);
				sscanf(fbuffer, "%s %s %s", rcode, status, sfsize);
				if (strcmp(rcode, "RRT") || n < (CODE_SIZE + 4))
					unexpected_protocol_FS();
				else{
					if (n == (CODE_SIZE + 4)){
						if (!strcmp(status, "EOF") || !strcmp(status, "NOK") || !strcmp(status, "INV") || !strcmp(status, "ERR")){
							write(1, "echo: ", 6); write(1, fbuffer, n);
						}
						else
							unexpected_protocol_FS();
					}
					else if (!strcmp(status, "OK")){
						if (strlen(sfsize) > 10)
							unexpected_protocol_FS();
						int n_offset = strlen(rcode) + 1 + strlen(status) + 1 + strlen(sfsize) + 1;
						fsize = atoi(sfsize);
						write(1, "echo: RRT OK\n", 13);
						printf("Retrieving ""%s""...\n", fname); //clean up later
						recv_file(fdFS, fname, fsize, fbuffer + n_offset, n - n_offset);
						printf("Done!\n");
					}
				}
			}
			else{
				n = read_buf(fdFS, buffer, sizeof(buffer));

				write(1, "echo: ", 6); write(1, buffer, n); //check if rcode/status are valid
				//read_buf rupl, rdel, rrem + stdout
			}

			freeaddrinfo(resFS);
			close(fdFS);
		}
	}

	return 0;
}
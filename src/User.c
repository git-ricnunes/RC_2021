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
#define CWD_SIZE 256
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
#define RAS_SIZE 11
#define RLS_SIZE 548 //3 + 1 + 2 + 15 * 25 + 15 * 11 + 1 + 1
#define RRT_SIZE 18
#define RFS_SIZE 10
#define STATUS_SIZE 6
#define MAX_FILES 15
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

void list_files(char * fbuf, int n){

	char lbuf[n+1];
	memset(lbuf, 0, n+1);
	strncpy(lbuf, fbuf, n);

	int num_files = 0;
	char * file_list[MAX_FILES+1];

	char * ftoken = strtok(lbuf, " \n");
	while (ftoken != NULL && num_files <= MAX_FILES){
		file_list[num_files++] = ftoken;
		ftoken = strtok(NULL, " \n");
	}
	if (num_files%2 != 0)
		unexpected_protocol_FS();

	printf("N\tFname\t\t\tFsize (bytes)\n");
	for (int i = 0; i < num_files; i+=2){
		if (strlen(file_list[i]) >= FNAME_SIZE || strlen(file_list[i+1]) >= FSIZE_SIZE)
			unexpected_protocol_FS();
		else
			printf("%d\t\t\t%s\t%s\n", i/2 + 1, file_list[i], file_list[i+1]);
	}
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

	//printf("Connected to Authentication Server ""%s"" in port %s\n", ipAS, portAS);

	FILE * fp;

	char cwd[CWD_SIZE];
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
	long fsize;
	char rcode[CODE_SIZE] = "";
	char status[STATUS_SIZE] = "";
	int session = LOGGED_OUT;
	int n_offset;

	srand(time(NULL));

	while(1){

		memset(cwd, 0, CWD_SIZE);
		memset(buffer, 0, BUFFER_SIZE);
		memset(fbuffer, 0, FBUFFER_SIZE);
		memset(msg, 0, BUFFER_SIZE);
		memset(fname, 0, FNAME_SIZE);
		memset(sfsize, 0, FSIZE_SIZE);
		memset(code, 0, CODE_SIZE);
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
				printf("Already logged in as UID %s\n", uid);
				continue;
			}
			sprintf(code, "LOG");
			if (strlen(token_list[1]) != UID_SIZE - 1){
				printf("Invalid UID: %s\n", token_list[1]);
				continue;
			}
			strcpy(uid, token_list[1]);
			if (strlen(token_list[2]) != PASS_SIZE - 1){
				printf("Invalid password: %s\n", token_list[2]);
				continue;
			}
			strcpy(pass, token_list[2]);
			sprintf(msg, "%s %s %s\n", code, uid, pass);
			fd = AS_FD_SET;
		}
		else if (!strcmp(token_list[0], "req")){ /* REQ UID RID Fop [Fname] */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				//printf("Login command: login UID pass\n");
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
					printf("Invalid file name: %s\n", token_list[2]);
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
				//printf("Login command: login UID pass\n");
				continue;
			}
			sprintf(code, "AUT");
			sprintf(msg, "%s %s %s %s\n", code, uid, rid, token_list[1]);
			fd = AS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "list") || !strcmp(token_list[0], "l")) && num_tokens == 1){ /* LST UID TID */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				//printf("Login command: login UID pass\n");
				continue;
			}
			if (!strcmp(tid, "0")){
				printf("Invalid TID: please request a file operation\n");
				continue;
			}
			sprintf(code, "LST");
			sprintf(msg, "%s %s %s\n", code, uid, tid);
			fd = FS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "retrieve") || !strcmp(token_list[0], "r")) && num_tokens == 2){ /* RTV UID TID Fname */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				//printf("Login command: login UID pass\n");
				continue;
			}
			if (!strcmp(tid, "0")){
				printf("Invalid TID: please request a file operation\n");
				continue;
			}
			sprintf(code, "RTV");
			if (strlen(token_list[1]) >= FNAME_SIZE){
				printf("Invalid file name: %s\n", token_list[2]);
				continue;
			}
			strcpy(fname, token_list[1]);
			fp = fopen(fname, "r");
			if (fp){
				printf("File %s already in current directory, please remove the file or request a different file name\n", fname);
				continue;
			}
			fclose(fp);
			sprintf(msg, "%s %s %s %s\n", code, uid, tid, fname);
			fd = FS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "upload") || !strcmp(token_list[0], "u")) && num_tokens == 2){ /* UPL UID TID Fname Fsize data */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				//printf("Login command: login UID pass\n");
				continue;
			}
			if (!strcmp(tid, "0")){
				printf("Invalid TID: please request a file operation\n");
				continue;
			}
			sprintf(code, "UPL");
			if (strlen(token_list[1]) >= FNAME_SIZE){
				printf("Invalid file name: %s\n", token_list[2]);
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
				printf("Could not determine file %s size\n", fname);
				continue;
			}
			if (fsize >= 10000000000){
				printf("File %s too large (%ld GB), should be lower than 10 GB\n", fname, fsize/1000000000);
				continue;
			}
			fclose(fp);
			sprintf(msg, "%s %s %s %s ", code, uid, tid, fname);
			fd = FS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "delete") || !strcmp(token_list[0], "d")) && num_tokens == 2){ /* DEL UID TID Fname */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				//printf("Login command: login UID pass\n");
				continue;
			}
			if (!strcmp(tid, "0")){
				printf("Invalid TID: please request a file operation\n");
				continue;
			}
			sprintf(code, "DEL");
			if (strlen(token_list[1]) >= FNAME_SIZE){
				printf("Invalid file name: %s\n", token_list[2]);
				continue;
			}
			strcpy(fname, token_list[1]);
			sprintf(msg, "%s %s %s %s\n", code, uid, tid, fname);
			fd = FS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "remove") || !strcmp(token_list[0], "x")) && num_tokens == 1){ /* REM UID TID */
			if (session == LOGGED_OUT){
				printf("Invalid command: please log in before attempting another command\n");
				//printf("Login command: login UID pass\n");
				continue;
			}
			if (!strcmp(tid, "0")){
				printf("Invalid TID: please request a file operation\n");
				continue;
			}
			sprintf(code, "REM");
			sprintf(msg, "%s %s %s\n", code, uid, tid);
			fd = FS_FD_SET;
		}
		else if (!strcmp(token_list[0], "exit") && num_tokens == 1){
			freeaddrinfo(resAS);
			close(fdAS);
			printf("Exited successfully.\n");
			exit(0);
		}
		else{
			printf("Invalid command\n");
			continue;
		}

		memset(buffer, 0, BUFFER_SIZE);
		
		if (fd == AS_FD_SET){

			write_buf(fdAS, msg, strlen(msg));
		
			n = read_buf(fdAS, buffer, BUFFER_SIZE, RAS_SIZE, NULL_IGNORE);

			sscanf(buffer, "%s %s", rcode, status);

			if (!strcmp(code, "LOG") && !strcmp(rcode, "RLO")){
				if (strcmp(status, "OK") && strcmp(status, "NOK") && strcmp(status, "ERR"))
					unexpected_protocol();
				if (!strcmp(status, "OK"))
					session = LOGGED_IN;
			}
			else if (!strcmp(code, "REQ") && !strcmp(rcode, "RRQ")){
				if (strcmp(status, "OK") && strcmp(status, "ELOG") && strcmp(status, "EPD") && strcmp(status, "EUSER") && strcmp(status, "EFOP") && strcmp(status, "ERR"))
					unexpected_protocol();
			}
			else if (!strcmp(code, "AUT") && !strcmp(rcode, "RAU")){
				if (strlen(status) >= TID_SIZE)
					unexpected_protocol();
				if (strcmp(status, "0"))
					strcpy(tid, status);
			}
			else
				unexpected_protocol();

			write(1, "AS: ", 4); write(1, buffer, n); //printf("AS: %s %s\n", rcode, status);
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

			//printf("Connected to File Server ""%s"" in port %s\n", ipFS, portFS);

			write_buf(fdFS, msg, strlen(msg));
			if (!strcmp(code, "UPL")){
				printf("Sending %s...\n", fname);
				send_file(fdFS, fname, SP_IGNORE);
			}

			if (!strcmp(code, "LST")){
				n = read_buf(fdFS, fbuffer, FBUFFER_SIZE, RLS_SIZE, NULL_IGNORE);
				sscanf(fbuffer, "%s %s", rcode, status);
				if (strcmp(rcode, "RLS") || n < (CODE_SIZE + 4))
					unexpected_protocol_FS();
				else{
					if (n == (CODE_SIZE + 4)){
						if (!strcmp(status, "EOF") || !strcmp(status, "NOK") || !strcmp(status, "INV") || !strcmp(status, "ERR")){
							write(1, "FS: ", 4); write(1, fbuffer, n);
						}
						else
							unexpected_protocol_FS();
					}
					else{
						n_offset = strlen(rcode) + 1 + strlen(status) + 1;
						write(1, "FS: RLS ", 8); write(1, status, strlen(status)); write(1, "\n", 1);
						list_files(fbuffer + n_offset, n - n_offset);
					}
				}
			}
			else if (!strcmp(code, "RTV")){
				n = read_buf(fdFS, fbuffer, FBUFFER_SIZE, RRT_SIZE, NULL_IGNORE);
				sscanf(fbuffer, "%s %s %s", rcode, status, sfsize);
				if (strcmp(rcode, "RRT") || n < (CODE_SIZE + 4))
					unexpected_protocol_FS();
				else{
					if (n == (CODE_SIZE + 4)){
						if (!strcmp(status, "EOF") || !strcmp(status, "NOK") || !strcmp(status, "INV") || !strcmp(status, "ERR")){
							write(1, "FS: ", 4); write(1, fbuffer, n);
						}
						else
							unexpected_protocol_FS();
					}
					else if (!strcmp(status, "OK")){
						if (strlen(sfsize) > 10)
							unexpected_protocol_FS();
						n_offset = strlen(rcode) + 1 + strlen(status) + 1 + strlen(sfsize) + 1;
						fsize = atol(sfsize);
						write(1, "FS: RRT OK\n", 11);
						printf("Retrieving %s...\n", fname);
						recv_file(fdFS, fname, fsize, fbuffer + n_offset, n - n_offset, F_NEXISTS);
						if (!getcwd(cwd, CWD_SIZE)){
							fprintf(stderr, "Error: failed to get current directory\n");
							fprintf(stderr, "Error code: %d\n", errno);
							exit(1);
						}
						printf("File %s successfully retrieved\n", fname);
						printf("PATH: %s\\%s SIZE: %ld\n", cwd, fname, fsize);
					}
				}
			}
			else{
				n = read_buf(fdFS, buffer, BUFFER_SIZE, RFS_SIZE, NULL_IGNORE);
				sscanf(buffer, "%s %s", rcode, status);
				if (!strcmp(code, "UPL") && !strcmp(rcode, "RUP")){
					if (strcmp(status, "OK") && strcmp(status, "NOK") && strcmp(status, "DUP") && strcmp(status, "FULL") && strcmp(status, "INV") && strcmp(status, "ERR"))
						unexpected_protocol_FS();
				}
				else if (!strcmp(code, "DEL") && !strcmp(rcode, "RDL")){
					if (strcmp(status, "OK") && strcmp(status, "NOK") && strcmp(status, "EOF") && strcmp(status, "INV") && strcmp(status, "ERR"))
						unexpected_protocol_FS();
				}
				else if (!strcmp(code, "REM") && !strcmp(rcode, "RRM")){
					if (strcmp(status, "OK") && strcmp(status, "NOK") && strcmp(status, "INV") && strcmp(status, "ERR"))
						unexpected_protocol_FS();
				}
				else
					unexpected_protocol_FS();

				write(1, "FS: ", 4); write(1, buffer, n); //printf("FS: %s %s\n", rcode, status);

				if (!strcmp(code, "REM")){
					freeaddrinfo(resFS);
					close(fdFS);
					freeaddrinfo(resAS);
					close(fdAS);
					printf("Exited successfully.\n");
					exit(0);
				}
			}

			memset(tid, 0, TID_SIZE);
			sprintf(tid, "0");

			freeaddrinfo(resFS);
			close(fdFS);
		}
	}
	return 0;
}
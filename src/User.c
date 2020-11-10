#include "msg.h"
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
#define UID_SIZE 64
#define PASS_SIZE 64
#define RID_SIZE 5
#define TID_SIZE 5
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

	char buffer[BUFFER_SIZE];
	char fbuffer[FBUFFER_SIZE];
	char msg[BUFFER_SIZE];
	char code[CODE_SIZE] = "";
	char uid[UID_SIZE] = "";
	char pass[PASS_SIZE] = "";
	char rid[RID_SIZE] = "";
	char tid[TID_SIZE] = "0";
	char status[STATUS_SIZE] = "";
	int session = LOGGED_OUT;

	srand(time(NULL));

	while(1){

		printf("Enter a command: ");
		fgets(buffer, BUFFER_SIZE, stdin);

		int num_tokens = 0;
		char * token_list[MAX_TOKENS];

		char * token = strtok(buffer, " \n");
		while (token != NULL){
			token_list[num_tokens++] = token;
			token = strtok(NULL, " \n");
		}

		int fd = 0;

		if (!strcmp(token_list[0], "login") && num_tokens == 3 && session == LOGGED_OUT){ /* LOG UID pass */
			sprintf(code, "LOG");
			strcpy(uid, token_list[1]);
			strcpy(pass, token_list[2]);
			sprintf(msg, "%s %s %s\n", code, uid, pass);
			fd = AS_FD_SET;
		}
		else if (!strcmp(token_list[0], "req")){ /* REQ UID RID Fop [Fname] */
			sprintf(code, "REQ");
			sprintf(rid, "%d%d%d%d", rand()%9, rand()%9, rand()%9, rand()%9);
			if ((!strcmp(token_list[1], "L") || !strcmp(token_list[1], "X")) && num_tokens == 2){
				sprintf(msg, "%s %s %s %s\n", code, uid, rid, token_list[1]);
				fd = AS_FD_SET;
			}
			else if ((!strcmp(token_list[1], "R") || !strcmp(token_list[1], "U") || !strcmp(token_list[1], "D")) && num_tokens == 3){
				sprintf(msg, "%s %s %s %s %s\n", code, uid, rid, token_list[1], token_list[2]);
				fd = AS_FD_SET;
			}
		}
		else if (!strcmp(token_list[0], "val") && num_tokens == 2){ /* AUT UID RID VC */
			sprintf(code, "AUT");
			sprintf(msg, "%s %s %s %s\n", code, uid, rid, token_list[1]);
			fd = AS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "list") || !strcmp(token_list[0], "l")) && num_tokens == 1){ /* LST UID TID */
			sprintf(code, "LST");
			sprintf(msg, "%s %s %s\n", code, uid, tid);
			fd = FS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "retrieve") || !strcmp(token_list[0], "r")) && num_tokens == 2){ /* RTV UID TID Fname */
			sprintf(code, "RTV");
			sprintf(msg, "%s %s %s %s\n", code, uid, tid, token_list[1]); /* inc: char Fname */
			fd = FS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "upload") || !strcmp(token_list[0], "u")) && num_tokens == 2){ /* UPL UID TID Fname Fsize data */
			sprintf(code, "UPL");
			/* inc */
			fd = FS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "delete") || !strcmp(token_list[0], "d")) && num_tokens == 2){ /* DEL UID TID Fname */
			sprintf(code, "DEL");
			sprintf(msg, "%s %s %s %s\n", code, uid, tid, token_list[1]); /* inc: char Fname */
			fd = FS_FD_SET;
		}
		else if ((!strcmp(token_list[0], "remove") || !strcmp(token_list[0], "x")) && num_tokens == 1){ /* REM UID TID */
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
			fprintf(stderr, "Invalid command\n");
			exit(1);
		}
		
		if (fd == AS_FD_SET){

			write_buf(fdAS, msg);
		
			n = read_buf(fdAS, buffer, sizeof(buffer));

			write(1, "echo: ", 6); write(1, buffer, n);

			sscanf(buffer, "%s %s", code, status);

			if (!strcmp(code, "RLO") && !strcmp(status, "OK")){
				session = LOGGED_IN;
			}
			else if (!strcmp(code, "RAU") && strcmp(status, "0")){
				strcpy(tid, status);
			}
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
		
			//

			freeaddrinfo(resFS);
			close(fdFS);
		}

		memset(buffer, 0, BUFFER_SIZE);
		memset(fbuffer, 0, FBUFFER_SIZE);
		memset(msg, 0, BUFFER_SIZE);
		memset(code, 0, CODE_SIZE);
	}

	return 0;
}
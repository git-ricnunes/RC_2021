#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define DEFAULT_ASIP "127.0.0.1"
#define DEFAULT_ASPORT "58011"
#define DEFAULT_FSIP "127.0.0.1"
#define DEFAULT_FSPORT "59011"
#define IP_SIZE 64
#define PORT_SIZE 16
#define BUFFER_SIZE 128
#define UID_SIZE 32
#define PASS_SIZE 32
#define MAX_TOKENS 4

int fd, fdAS, fdFS, errcode;
ssize_t n;
socklen_t addrlenAS, addrlenFD;
struct addrinfo hintsAS, *resAS, hintsFD, *resFD;
struct sockaddr_in addrAS, addrFD;

int main(int argc, char *argv[]){

	char ipAS[IP_SIZE] = DEFAULT_ASIP, portAS[PORT_SIZE] = DEFAULT_ASPORT, ipFS[IP_SIZE] = DEFAULT_FSIP, portFS[PORT_SIZE] = DEFAULT_FSPORT;

	for (int i = 1; i < argc; i++){
		if (argv[i][0] != '-' || strlen(argv[i]) != 2) continue;
		switch (argv[i][1]){
			case 'n':
				strcpy(ipAS, argv[i+1]);
				break;
			case 'p':
				strcpy(portAS, argv[i+1]);
				break;
			case 'm':
				strcpy(ipFS, argv[i+1]);
				break;
			case 'q':
				strcpy(portFS, argv[i+1]);
				break;
			default: /* invalid flag */
				continue;
		}
	}

	fdAS=socket(AF_INET,SOCK_STREAM,0);
	if(fdAS==-1) exit(1);
		
	memset(&hintsAS,0,sizeof hintsAS);
	hintsAS.ai_family=AF_INET;
	hintsAS.ai_socktype=SOCK_STREAM;
		
	errcode = getaddrinfo(ipAS,portAS,&hintsAS,&resAS);
	if(errcode!=0) exit(1);

	n=connect(fdAS,resAS->ai_addr,resAS->ai_addrlen);
	if(n==-1) exit(1);

	char buffer[BUFFER_SIZE];
	char uid[UID_SIZE] = "";
	char pass[PASS_SIZE] = "";
	int rid = 9999; // rng/list?

	while(1){

		fgets(buffer, BUFFER_SIZE, stdin);

		int num_tokens = 0;
		char * token_list[MAX_TOKENS];

		char * token = strtok(buffer, " \n");
		while (token != NULL){
			token_list[num_tokens++] = token;
			token = strtok(NULL, " \n");
		}

		char msg[BUFFER_SIZE] = "";
		if (!strcmp(token_list[0], "login") && num_tokens == 3){
			sprintf(msg, "LOG %s %s\n", token_list[1], token_list[2]);
			fd = fdAS;
		}
		else if (!strcmp(token_list[0], "req") && num_tokens == 3){ //RID rng?
			sprintf(msg, "REQ %s %d %s %s\n", uid, rid, token_list[1], token_list[2]);
			fd = fdAS;
		}
		else if (!strcmp(token_list[0], "val") && num_tokens == 2){
			sprintf(msg, "AUT %s %d %s\n", uid, rid, token_list[1]);
			fd = fdAS;
		}
		else if ((!strcmp(token_list[0], "list") || !strcmp(token_list[0], "l")) && num_tokens == 1){
			fd = fdFS;
		}
		else if ((!strcmp(token_list[0], "retrieve") || !strcmp(token_list[0], "r")) && num_tokens == 2){
			fd = fdFS;
		}
		else if ((!strcmp(token_list[0], "upload") || !strcmp(token_list[0], "u")) && num_tokens == 2){
			fd = fdFS;
		}
		else if ((!strcmp(token_list[0], "delete") || !strcmp(token_list[0], "d")) && num_tokens == 2){
			fd = fdFS;
		}
		else if ((!strcmp(token_list[0], "remove") || !strcmp(token_list[0], "x")) && num_tokens == 1){
			fd = fdFS;
		}
		else if (!strcmp(token_list[0], "exit") && num_tokens == 1){
			freeaddrinfo(resAS);
			close(fdAS);
			break;
		}
		else{
			continue; //invalid command (?)
		}
		
		n=write(fd,msg,strlen(msg));
		if(n==-1) exit(1);
		
		n=read(fd,buffer,sizeof(buffer));
		if(n==-1) exit(1);
		
		write(1,"echo: ",6); write(1,buffer,n);
		//if rlo ok >> guardar uid e pass
	}
	return 0;
}
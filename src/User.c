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
#define MAX_TOKENS 4

#include <errno.h>

int fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[BUFFER_SIZE];

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

	char * token_list[MAX_TOKENS];
	int num_tokens;

	while(1){

		fgets(buffer, BUFFER_SIZE, stdin);

		char msg[BUFFER_SIZE] = "";
		char * token = strtok(buffer, " \n");

		while (token != NULL){
			token_list[num_tokens++] = token;
			token = strtok(NULL, " \n");
		}

		if (!strcmp(token_list[0], "login")){
			sprintf(msg, "LOG %s %s\n", token_list[1], token_list[2]);
		}
		else if (!strcmp(token_list[0], "req")){

		}
		else if (!strcmp(token_list[0], "val")){

		}
		else if (!strcmp(token_list[0], "list") || !strcmp(token_list[0], "l")){

		}
		else if (!strcmp(token_list[0], "retrieve") || !strcmp(token_list[0], "r")){

		}
		else if (!strcmp(token_list[0], "upload") || !strcmp(token_list[0], "u")){

		}
		else if (!strcmp(token_list[0], "delete") || !strcmp(token_list[0], "d")){

		}
		else if (!strcmp(token_list[0], "remove") || !strcmp(token_list[0], "x")){

		}
		else if (!strcmp(token_list[0], "exit")){

		}

		fd=socket(AF_INET,SOCK_STREAM,0);
		if(fd==-1) exit(1);
		
		memset(&hints,0,sizeof hints);
		hints.ai_family=AF_INET;
		hints.ai_socktype=SOCK_STREAM;
		
		errcode = getaddrinfo(ipAS,portAS,&hints,&res);
		if(errcode!=0) exit(1);

		n=connect(fd,res->ai_addr,res->ai_addrlen);
		if(n==-1) exit(1);
		
		n=write(fd,msg,strlen(msg));
		if(n==-1) exit(1);
		
		n=read(fd,buffer,sizeof(buffer));
		if(n==-1) exit(1);
		
		write(1,"echo: ",6); write(1,buffer,n);
		
		freeaddrinfo(res);
		close(fd);
	}
	return 0;
}
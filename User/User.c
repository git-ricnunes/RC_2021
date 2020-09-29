#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#define DEFAULT_ASIP "127.0.0.1"
#define DEFAULT_ASPORT "58011"
#define DEFAULT_FSIP "127.0.0.1"
#define DEFAULT_FSPORT "59011"

int fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128];

int main(int argc, char *argv[]){

	char ipAS[18] = DEFAULT_ASIP, portAS[8] = DEFAULT_ASPORT, ipFS[18] = DEFAULT_FSIP, portFS[8] = DEFAULT_FSPORT;

	for (int i = 1; i < argc; i++){
		if (argv[i][0] != '-' || strlen(argv[i]) != 2) continue;
		switch (argv[i][1]){
			case 'n':
				strcpy(ipAS, argv[i+1]);
			case 'p':
				strcpy(portAS, argv[i+1]);
			case 'm':
				strcpy(ipFS, argv[i+1]);
			case 'q':
				strcpy(portFS, argv[i+1]);
			default: /* invalid flag */
				continue;
		}
	}

	while(fgets(buffer, 128, stdin)){
		
		fd=socket(AF_INET,SOCK_STREAM,0);
		if(fd==-1) exit(1);
		
		memset(&hints,0,sizeof hints);
		hints.ai_family=AF_INET;
		hints.ai_socktype=SOCK_STREAM;
		
		errcode = getaddrinfo(ipAS,portAS,&hints,&res);
		
		n=connect(fd,res->ai_addr,res->ai_addrlen);
		
		n=write(fd,buffer,sizeof(buffer));
		
		n=read(fd,buffer,sizeof(buffer));
		
		write(1,"echo: ",6); write(1,buffer,n);
		
		freeaddrinfo(res);
		close(fd);
	}
	return 0;
}
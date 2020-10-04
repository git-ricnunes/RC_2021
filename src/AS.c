#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <netinet/in.h>
#define PORT "57011"

int fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128];

/**
* TODO
* As of today work as a simple udp cliente that sends a msg to Personal device.
*
*/
int main(int argc, char *argv[]){
	

	fd=socket(AF_INET,SOCK_DGRAM,0);
	if(fd==-1) exit(1);
	
	memset(&hints,0,sizeof hints);
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_DGRAM;
	
	errcode = getaddrinfo("sigma01.ist.utl.pt",PORT,&hints,&res);
	
	n=sendto(fd,"VLC 71015 1234 L test.txt\n",26,0,res->ai_addr,res->ai_addrlen);

	addrlen = sizeof(addr);
	n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
	
	write(1,"echo: ",6); write(1,buffer,n);
	
	freeaddrinfo(res);
	close(fd);
	return 0;
}

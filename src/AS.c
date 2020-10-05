#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include <netinet/in.h>
#define DEFAULT_PORT_AS "58011"
#define max(A,B) ((A)>=(B)?(A):(B))

int fds,fd,errcode;
ssize_t ns,n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128];

/**
* TODO
* As of today work as a simple udp cliente that sends a msg to Personal device.
*
*/

struct user
{
    char uid[6];
    char pass[9];
	char pdIp[18];	
    char pdPort[6];
};


int main(int argc, char *argv[]){
	
	char portAS[6]=DEFAULT_PORT_AS;
	int verboseMode=0;
	fd_set rfds;
	int maxfd,retval;
	struct user arr_user[20];
	int numUsers=0;
	
	
	if( argc > 3){
		printf("Error in argument setting!\n");
		return -1;
	}
	
	else if(argc == 4){
		
		if(strcmp(argv[1],"-p") ==0){
			stpcpy(portAS,argv[2]);
		} else if(strcmp(argv[2],"-p") ==0){
			stpcpy(portAS,argv[3]);
		}
		
		if(strcmp(argv[1],"-v") ==0){
			verboseMode=1;
		} else if(strcmp(argv[3],"-p") ==0){
			verboseMode=1;
		}
		
	}
	
	// Create the udp server socket
	if((fds=socket(AF_INET,SOCK_DGRAM,0))==-1){
		printf("Error: unable to create udp server socket\n");
		printf("Error code: %d\n", errno);
		exit(1);
	}
		
	memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_flags=AI_PASSIVE;
  
    errcode=getaddrinfo(NULL,portAS,&hints,&res);
    if(errcode!=0) exit(1);

    ns=bind(fds,res->ai_addr,res->ai_addrlen);
	if(ns==-1) {  
		printf("Error: unable to bind the udp server socket\n");
		printf("Error code: %d\n", errno);
		exit(1);
		}
		
	// Create the udp client socket
	if((fd=socket(AF_INET,SOCK_DGRAM,0))==-1){
		printf("Error: unable to create udp client socket\n");
		printf("Error code: %d\n", errno);
		exit(1);
	} 

	memset(&hints,0,sizeof hints);
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_DGRAM;
		
	errcode = getaddrinfo("127.0.0.1",portAS,&hints,&res);
	
		
	 while(1){
		
			// setting the select vars
			FD_SET(0,&rfds);
			FD_SET(fd,&rfds);
			FD_SET(fds,&rfds);
			maxfd = max(fd,fds);	
			
			// reset the message buffers in each iterarion
			char msg[128]="";
			char buffer[128];
			
			retval=select(maxfd+1,&rfds,(fd_set *)NULL,(fd_set *)NULL,(struct timeval *) NULL);
			if(maxfd<=0)exit(1);
			
			for(;retval;retval--){
				
				if(FD_ISSET(fd,&rfds)){	
					// client udp
					// not sure if needed
					
				} else if(FD_ISSET(fds,&rfds)){
					//server udp
					
				  char op[4];
				  char user[6];
				  char pass[9];
				  char pdIP[6];
				  char pdPort[9];
				  addrlen = sizeof(addr);
				  
				  n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
				  write(1,buffer,n);
		
				  // TODO VERBOSE MODE
				  
				  sscanf( buffer, "%s %s %s %s %s", op, user, pass, pdIP, pdPort);				  
				
				}					
				
			}
			
			
			
		
	 }
		
		
		
	return 0;
}

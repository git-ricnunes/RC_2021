#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <netinet/in.h>

#define DEFAULT_PORT_PD "57011"
#define DEFAULT_PORT_AS "58011"
#define DEFAULT_IP_AS "127.0.0.1"
#define DEFAULT_IP_PD "127.0.0.1"


int fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128];

int main(int argc, char *argv[]){
	
	// Variables
	char portPD[8] = DEFAULT_PORT_PD;
	char portAS[8] = DEFAULT_PORT_AS;
	char ipPD[18] = DEFAULT_IP_PD ;
	char ipAS[18] = DEFAULT_IP_AS;
	
	if( argc == 1){
		printf("The PDIP argument is required!\n");
		return -1;
	}
	else if(argc == 2){
		stpcpy(argv[1],ipPD);
	
	}
	else if(argc == 4){
		
		stpcpy(ipPD,argv[1]);

		if(strcmp(argv[2],"-d") ==0){
			stpcpy(portPD,argv[3]);
		} else if(strcmp(argv[2],"-n") ==0){
			stpcpy(ipAS,argv[3]);
		}else if(strcmp(argv[2],"-p") ==0){
			stpcpy(portAS,argv[3]);
		}
	}
	else if(argc == 6){	
	
		stpcpy(ipPD,argv[1]);

		if(strcmp(argv[2],"-d") ==0){
			stpcpy(portPD,argv[3]);
		} else if(strcmp(argv[2],"-n") ==0){
			stpcpy(ipAS,argv[3]);
		}else if(strcmp(argv[2],"-p") ==0){
			stpcpy(portAS,argv[3]);
		} 
			
		if(strcmp(argv[4],"-d") ==0){
			stpcpy(portPD,argv[5]);
		}else if(strcmp(argv[4],"-n") ==0){
			stpcpy(ipAS,argv[5]);
		}else if(strcmp(argv[4],"-p") ==0){
			stpcpy(portAS,argv[5]);
		}
	}
	else if(argc == 8){
		stpcpy(ipPD,argv[1]);
		
		if(strcmp(argv[2],"-d") ==0){
			stpcpy(portPD,argv[3]);
		} else if(strcmp(argv[2],"-n") ==0){
			stpcpy(ipAS,argv[3]);
		}else if(strcmp(argv[2],"-p") ==0){
			stpcpy(portAS,argv[3]);
		}

		if(strcmp(argv[4],"-d") ==0){
			stpcpy(portPD,argv[5]);
		}else if(strcmp(argv[4],"-n") ==0){
			stpcpy(ipAS,argv[5]);
		}else if(strcmp(argv[4],"-p") ==0){
			stpcpy(portAS,argv[5]);
		}

		if(strcmp(argv[6],"-d") ==0){
			stpcpy(portPD,argv[7]);
		}else if(strcmp(argv[6],"-n") ==0){
			stpcpy(ipAS,argv[7]);
		}else if(strcmp(argv[6],"-p") ==0){
			stpcpy(portAS,argv[7]);
		}
	} else {
		printf("Erros in arguments!\n");
		return -1;
	}

    while(fgets(buffer, 128 , stdin)){

		fd=socket(AF_INET,SOCK_DGRAM,0);
		if(fd==-1) exit(1);
		
		memset(&hints,0,sizeof hints);
		hints.ai_family=AF_INET;
		hints.ai_socktype=SOCK_DGRAM;
		
		errcode = getaddrinfo(ipAS,portAS,&hints,&res);
		
		n=sendto(fd,"REG 71015 password 127.0.0.1 57011\n",35,0,res->ai_addr,res->ai_addrlen);
		
		addrlen = sizeof(addr);
		n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
		
		write(1,"echo: ",6); write(1,buffer,n);
		
		freeaddrinfo(res);
		close(fd);
	}
	return 0;
	
	
	
	
}

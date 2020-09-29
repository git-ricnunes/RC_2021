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

#define max(A,B) ((A)>=(B)?(A):(B))

int fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;

int main(int argc, char *argv[]){
	
	// Variables
	char portPD[8] = DEFAULT_PORT_PD;
	char portAS[8] = DEFAULT_PORT_AS;
	char ipPD[18] = DEFAULT_IP_PD ;
	char ipAS[18] = DEFAULT_IP_AS;
	
	// select vars
	/*
	int fd,newfd,afd=0;
	fd_set rfds;
	enum {idle,busy} state;
	int maxfd,counter;
	*/
	
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

    char *token_list[20]; 
	
	char user[6] ="";
	char pass[9] ="";
	int _exit =0;

    while(1){
		
		char msg[128] = "";
		char buffer[128];
		
		printf("Enter your command (reg or exit):\n");
		fgets(buffer, 128 , stdin);
		buffer[strlen(buffer)-1]='\0';

		char* token = strtok(buffer, " ");
		int num_tokens = 0;
		
		while (token != NULL) { 
			token_list[num_tokens] = token;
			num_tokens++;
			token = strtok(NULL, " ");
		} 

		if(strcmp(token_list[0],"reg")==0){
			
			strcat(msg,"REG ");
			strcat(msg,token_list[1]);
			strcat(msg," ");
			strcat(msg,token_list[2]);
			strcat(msg," ");
			strcat(msg,ipPD);
			strcat(msg," ");
			strcat(msg,portPD);
			strcat(msg,"\n");
						
			strcpy(user,token_list[1]);
			strcpy(pass,token_list[2]);		

		}else if(strcmp(token_list[0],"exit")==0){
			
			strcat(msg,"UNR ");
			strcat(msg,user);
			strcat(msg," ");
			strcat(msg,pass);
			strcat(msg,"\n");
			
			_exit=1;
		}	
			
		fd=socket(AF_INET,SOCK_DGRAM,0);
		if(fd==-1) exit(1);
		
		memset(&hints,0,sizeof hints);
		hints.ai_family=AF_INET;
		hints.ai_socktype=SOCK_DGRAM;
		
		errcode = getaddrinfo(ipAS,portAS,&hints,&res);
		
		n=sendto(fd,msg,strlen(msg),0,res->ai_addr,res->ai_addrlen);
		
		addrlen = sizeof(addr);
		n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
		
		write(1,"echo: ",6); write(1,buffer,n);
		
		freeaddrinfo(res);
		close(fd);
		
		if(_exit==1) return 0;
	}
	return 0;
}

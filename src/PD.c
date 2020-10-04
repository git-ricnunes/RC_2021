#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>

#define DEFAULT_PORT_PD "57011"
#define DEFAULT_PORT_AS "58011"
#define DEFAULT_IP_AS "127.0.0.1"
#define DEFAULT_IP_PD "127.0.0.1"

#define max(A,B) ((A)>=(B)?(A):(B))

int fd,fds,errcode;
ssize_t n,ns;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;

int main(int argc, char *argv[]){
	
	// Variables
	char portPD[6] = DEFAULT_PORT_PD;
	char portAS[6] = DEFAULT_PORT_AS;
	char ipPD[18] = DEFAULT_IP_PD ;
	char ipAS[18] = DEFAULT_IP_AS;
 
	char user[6] ="";
	char pass[9] ="";
	
	fd_set rfds;
	int maxfd,retval;

	// Input arguments processment
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
	
	for(int i = 0 ; i< argc; i++)
		
	
	addrlen=sizeof(addr);
	
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
  
    errcode=getaddrinfo(NULL,DEFAULT_PORT_PD,&hints,&res);
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
		
	errcode = getaddrinfo(ipAS,portAS,&hints,&res);
	
    while(1){
		
			// setting the
			FD_ZERO(&rfds);
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
      
  			if(FD_ISSET(0,&rfds))
  			{
        fgets(buffer, 128 , stdin);
        
        char op[4];
        
          sscanf( buffer, "%s %s %s", op, user, pass );

  				if(strcmp(op,"reg")==0){
  					
  					strcat(msg,"REG ");
  					strcat(msg,user);
  					strcat(msg," ");
  					strcat(msg,pass);
  					strcat(msg," ");
  					strcat(msg,ipPD);
  					strcat(msg," ");
  					strcat(msg,portPD);
  					strcat(msg,"\n");
  				}else if(strcmp(op,"exit")==0){
  					strcat(msg,"UNR ");
  					strcat(msg,user);
  					strcat(msg," ");
  					strcat(msg,pass);
  					strcat(msg,"\n");
				} 
        
  				n=sendto(fd,msg,strlen(msg),0,res->ai_addr,res->ai_addrlen);
  
				} else if(FD_ISSET(fd,&rfds)){			
        
				  char op[4];
				  char status[4];
				  
				  n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
				  write(1,buffer,n);
				  sscanf( buffer, "%s %s", op, status);    
				  
				  if(strcmp(op,"RUN")==0){
  						if(strcmp(status,"OK")==0)
							return 0;
						} else {
							strcat(msg,"ERR");
							strcat(msg,"\n");
						}		
					} else if(FD_ISSET(fds,&rfds)){
						char op[4];
						char userAs[7];
						char onTimeCode[5];
						char fileOp[2];
						char fileName[100];	
						
						addrlen = sizeof(addr);
						ns=recvfrom(fds,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
						if(ns==-1) exit(1);
						
						write(1,buffer,ns);
						
						sscanf( buffer,"%s %s %s %s %s", op,userAs,onTimeCode,fileOp,fileName); 
                
					if(strcmp(op,"VLC")==0){
  						if(strcmp(userAs,user)==0){                                   
    						strcat(msg,"RVC ");
    						strcat(msg,"OK");
    						strcat(msg,"\n");
    						
						} else {	
    						strcat(msg,"RVC ");
    						strcat(msg,"NOK");
    						strcat(msg,"\n");
  						}
					} else {
						strcat(msg,"ERR");
						strcat(msg,"\n");
						}	
					ns=sendto(fds,msg,strlen(msg),0,(struct sockaddr*) &addr,addrlen);
                                                  
					if(ns==-1){
						printf("Error: unable to send the message socket\n");
						printf("Error code: %d\n", errno);
						exit(1);
						} 
  				}		
  			}
      }
	return 0;
}

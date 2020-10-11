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
#define MAX_NUM_U 30

int fds,fd,errcode;
ssize_t ns,n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128];

struct user_st
{
    char uid[6];
    char pass[9];
	  char pdIp[17];	
    char pdPort[6];
};

int main(int argc, char *argv[]){
	
	char portAS[6]=DEFAULT_PORT_AS;
	int verboseMode=0;
	fd_set rfds;
	int maxfd,retval;
	struct user_st arr_user[MAX_NUM_U];
	int numUsers=-1;
  addrlen = sizeof(addr);
	
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
			FD_SET(fd,&rfds);
			FD_SET(fds,&rfds);
			maxfd = max(fd,fds);	
			
			// reset the message buffers in each iterarion
			char msg[128]="";
      char buffer[128]="";
			
			retval=select(maxfd+1,&rfds,(fd_set *)NULL,(fd_set *)NULL,(struct timeval *) NULL);
			if(maxfd<=0)exit(1);
			
			for(;retval;retval--){
				
			if(FD_ISSET(fds,&rfds)){
					//server udp
                   
				  char op[5]="";
				  char user[6]="";
				  char pass[9]="";
				  char pdIP[17]="";
				  char pdPort[6]="";
				  
				  ns=recvfrom(fds,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
                                                 
				  write(1,buffer,ns);
                               
				  sscanf(buffer,"%s %s %s %s %s", op, user, pass,pdIP,pdPort);              
                           
          if(strcmp(op,"REG")==0){
            
            if(strlen(user)== 5 && strlen(pass)==8 && strcmp(pdIP,"")!=0 && strcmp(pdPort,"")!=0){
            
              struct user_st st_u;
              int userUpdate=0;
              int oldNumUsers = numUsers;
              
              for(int i = 0; i < numUsers;i++){
                if(strcmp(arr_user[i].uid,user)==0){
                  numUsers=i;
                  userUpdate=1;
                  break;
                }
              }
              
              if(userUpdate==0){
                 numUsers++;
              }
            
              strcpy(st_u.uid,user);
              strcpy(st_u.pass,pass);
              strcpy(st_u.pdIp,pdIP);
              strcpy(st_u.pdPort,pdPort);
              
              arr_user[numUsers]=st_u;
              
               if(userUpdate!=0){
                 numUsers=oldNumUsers;
              }
              
              printf("struct checker: %d\n",numUsers);
              for(int j = 0; j <= numUsers;j++)
                printf("->%s %s %s %s \n",arr_user[j].uid,arr_user[j].pass,arr_user[numUsers].pdIp,arr_user[j].pdPort);
              
             	strcat(msg,"RRG ");
              strcat(msg,"OK");
              strcat(msg,"\n");
              
        		  ns=sendto(fds,msg,strlen(msg),0,(struct sockaddr*) &addr,addrlen);             
              
            }else {
            
              strcat(msg,"RRG ");
              strcat(msg,"NOK");
              strcat(msg,"\n");
              
              ns=sendto(fds,msg,strlen(msg),0,(struct sockaddr*) &addr,addrlen);  
            
            }		  
				
				}else if(strcmp(op,"UNR")==0){ 
        
              struct user_st st_u;
              int deleteUser=0;
              int deleteUserIndex=0;
              
              for(int i = 0; i < MAX_NUM_U;i++){
                if(strcmp(arr_user[i].uid,user)==0 && strcmp(arr_user[i].pass,pass)==0){
                  deleteUser=1;
                  deleteUserIndex=i;
                  numUsers--;
                  break;
                }
              }
              if(deleteUser){
              
                for(int i = deleteUserIndex; i < numUsers;i++){
                     arr_user[i]=arr_user[i+1];
                     }
   
                printf("struct checker: %d\n",numUsers);
                for(int j = 0; j <= numUsers;j++)
                  printf("->%s %s %s %s \n",arr_user[j].uid,arr_user[j].pass,arr_user[numUsers].pdIp,arr_user[j].pdPort);
                  
                strcat(msg,"RUN ");
                strcat(msg,"OK");
                strcat(msg,"\n");
                
                ns=sendto(fds,msg,strlen(msg),0,(struct sockaddr*) &addr,addrlen); 
          
              } else {
              
                strcat(msg,"RUN ");
                strcat(msg,"NOK");
                strcat(msg,"\n");
                
                ns=sendto(fds,msg,strlen(msg),0,(struct sockaddr*) &addr,addrlen); 
              
              }
          }else{
          
            strcat(msg,"ERR");
            strcat(msg,"\n");
            ns=sendto(fds,msg,strlen(msg),0,(struct sockaddr*) &addr,addrlen);  
          }					
    				
			}
   }
  }

  return 0;
}

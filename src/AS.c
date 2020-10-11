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

int fds,fd,tcp_fd,tcp_accept_fd,errcode;
ssize_t ns,n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct addrinfo hints_client_udp,*res_client_udp;
struct sockaddr_in addr;
char buffer[128];
char tcp_buffer[128];
char tcp_msg[128];
char udp_msg[128];
char udp_buffer[128];
int numUsers=-1;

struct user_st
{
    char uid[6];
    char pass[9];
	char pdIp[17];	
    char pdPort[6];
	int  isLogged;
	int  RID;
};


int checkFileOp(char *opOut){
	
	int result = 0;
	char fileOps[5]={'L','R','U','D','X'};
	
	for(int i=0;i<5;i++)
		if(opOut[0]==fileOps[i]){
			result=1;
			break;
		}
			
	return result;
	
}

void structChecker(struct user_st *arr_user){
	
	 printf("struct checker: %d\n",numUsers);
	 for(int j = 0; j <= numUsers;j++)
		 printf("->%s %s %s %s %d %d\n",arr_user[j].uid,arr_user[j].pass,arr_user[j].pdIp,arr_user[j].pdPort,arr_user[j].isLogged,arr_user[j].RID);
}

int checkReqErr(char *tcp_buffer){
	
	int result = 0;
	int num_tokens=0;
	char testBuffer[30];
	char * token;
	
	strcpy(testBuffer,tcp_buffer);
	token = strtok(testBuffer, " ");
	
	while (token != NULL){
			num_tokens++;
			token = strtok(NULL, "\n");
		}
		
		if(num_tokens!=5)
			result=1;
			
	return result;
	
}

int main(int argc, char *argv[]){
	
	char portAS[6]=DEFAULT_PORT_AS;
	int verboseMode=0;
	fd_set rfds;
	int maxfd,maxfd1,retval;
	struct user_st arr_user[MAX_NUM_U];
	
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
	
	memset(&hints_client_udp,0,sizeof hints_client_udp);
	hints_client_udp.ai_family=AF_INET;
	hints_client_udp.ai_socktype=SOCK_DGRAM;
				
				
	// Create the tcp server socket
	if((tcp_fd=socket(AF_INET,SOCK_STREAM,0))==-1){
		printf("Error: unable to create tcp server socket\n");
		printf("Error code: %d\n", errno);
		exit(1);
	}
	
	memset(&hints,0,sizeof hints);
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags=AI_PASSIVE;
		     
	errcode = getaddrinfo(NULL,portAS,&hints,&res);
	if(errcode==-1) {  
  		printf("Error: unable to bind the tcp server socket\n");
  		printf("Error code: %d\n", errno);
  		exit(1);
  		}
		
	n=bind(tcp_fd,res->ai_addr,res->ai_addrlen);
	if(ns==-1) {  
  		printf("Error: unable to bind the tcp server socket\n");
  		printf("Error code: %d\n", errno);
  		exit(1);
  		}
	if(listen(tcp_fd,5) ==-1){
		printf("Error: unable to set the server to listen\n");
  		printf("Error code: %d\n", errno);
  		exit(1);
	}
	
	 while(1){
		
			// setting the select vars
			FD_SET(fd,&rfds);
			FD_SET(fds,&rfds);
			FD_SET(tcp_fd,&rfds);
			maxfd1 = max(fd,fds);
			maxfd  = max(maxfd1,tcp_fd);			
			 
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
			  st_u.isLogged=0;
			  st_u.RID=0;
              
              arr_user[numUsers]=st_u;
              
               if(userUpdate!=0){
                 numUsers=oldNumUsers;
              }
              
              structChecker(arr_user);
			  
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
   
				structChecker(arr_user);
			   
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
		  
		}else if(FD_ISSET(tcp_fd,&rfds)){
			
			char op[5]="";
			char user[6]="";
			char arg1[9]="";
			char arg2[10]="";
			char arg3[10]="";
			char status[10]="";
			int uindex=0;
			
			
			if((tcp_accept_fd=accept(tcp_fd,(struct sockaddr*)&addr,&addrlen))==-1){
				printf("%s %s\n","error accept:",strerror(errno));
				close(tcp_fd);
				exit(1);
			}
			
			n=read(tcp_accept_fd,tcp_buffer,sizeof(tcp_buffer));
			
			sscanf(tcp_buffer,"%s %s %s %s %s", op, user, arg1,arg2,arg3);  
			
			struct user_st st_u;
			
			strcpy(st_u.uid,"");
			
			//structChecker(arr_user);


			 if(strcmp(op,"LOG")==0){
				 
				
				// ESTRUTURA DE DADOS PARA O USER
				
				for(int i =0;i<= numUsers; i++ ){
				

					if(strcmp(arr_user[i].uid,user)==0){
							
							st_u = arr_user[i];
							uindex=i;

							break;
					}	
				}
				//printf("input checker %d ,%s",strcmp(st_u.uid,""),st_u.uid);

				
				if(strcmp(st_u.uid,"")==0){
					
					strcat(tcp_msg,"RLO ");
					strcat(tcp_msg,"ERR");
					strcat(tcp_msg,"\n");
				
				} else if(strcmp(st_u.pass,arg1)!=0){
					
							strcat(tcp_msg,"RLO ");
							strcat(tcp_msg,"NOK");
							strcat(tcp_msg,"\n");
							
						} else {
						
							arr_user[uindex].isLogged=1;
							strcat(tcp_msg,"RLO ");
							strcat(tcp_msg,"OK");
							strcat(tcp_msg,"\n");
							structChecker(arr_user);

					}
				
			 }else if(strcmp(op,"REQ")==0){
				 				
				struct user_st st_u;
								
				// ESTRUTURA DE DADOS PARA O USER - rever na aula
				
				for(int i =0;i<= numUsers; i++ ){
					if(strcmp(arr_user[i].uid,user)==0){
							
							st_u = arr_user[i];
							break;
					}	
				}
				
				structChecker(arr_user);

				 if(checkReqErr(tcp_buffer)==0){

						strcat(tcp_msg,"ERR");
							
				} else if(strcmp(st_u.uid,"")==0){
					
					strcat(tcp_msg,"RRQ ");
					strcat(tcp_msg,"EUSER");
					strcat(tcp_msg,"\n");
				
					} else {
						
						strcat(udp_msg,"VLC ");
						strcat(udp_msg,user);
						strcat(udp_msg," ");
						strcat(udp_msg,arg1);
						strcat(udp_msg," ");
						strcat(udp_msg,arg2);
						strcat(udp_msg," ");
						strcat(udp_msg,arg3);
						strcat(udp_msg,"\n");
									
						errcode = getaddrinfo(st_u.pdIp,st_u.pdPort,&hints,&res);
			
						n=sendto(fd,udp_msg,strlen(udp_msg),0,res->ai_addr,res->ai_addrlen);
									
						n=recvfrom(fd,udp_buffer,strlen(udp_buffer),0,(struct sockaddr*)&addr,&addrlen);
						
						
						write(1,"echo from PD: ",14); write(1,udp_buffer,n);
									
						sscanf(udp_buffer,"%s %s", op, status); 
						
						structChecker(arr_user);
									
						if(st_u.isLogged==0){
							
							strcat(tcp_msg,"RRQ ");
							strcat(tcp_msg,"ELOG");
							strcat(tcp_msg,"\n");
							
						} else if(strcmp(status,"NOK")==0) {

								strcat(tcp_msg,"RRQ ");
								strcat(tcp_msg,"EPD");
								strcat(tcp_msg,"\n");		
								
						} else if(checkFileOp(arg2)==0){
									
								strcat(tcp_msg,"RRQ ");
								strcat(tcp_msg,"EFOP");
								strcat(tcp_msg,"\n");
								
						}  else {
							
							strcat(tcp_msg,"RRQ ");
							strcat(tcp_msg,"OK");
							strcat(tcp_msg,"\n");
						}
					}
				 
			 }else if(strcmp(op,"AUT")==0){
				 
				strcat(tcp_msg,"RAU ");
				strcat(tcp_msg,"OK/NOK");
				strcat(tcp_msg,"\n");
				 
			 }else {
				 strcat(tcp_msg,"ERR");
				 strcat(tcp_msg,"\n");
			 }

			n=write(tcp_accept_fd,tcp_msg,sizeof(tcp_msg));
			close(tcp_accept_fd);
			
			memset(buffer,0,strlen(buffer));
			memset(tcp_msg,0,strlen(tcp_msg));
			memset(udp_msg,0,strlen(udp_msg));
			memset(udp_buffer,0,strlen(udp_buffer));
			
			
		}
		
		
   }
  }

  return 0;
}


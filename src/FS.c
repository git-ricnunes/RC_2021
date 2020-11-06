#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define IP_AS_DEFAULT "127.0.0.1"
#define PORT_AS_DEFAULT "58011"
#define PORT_FS_DEFAULT "59011"

/*verbose mode on:
servidor imprime descricao dos pedidos recebidos e o IP e porta desses pedidos
*/
int verbose_mode = 0;
//Numero de mensagens de verificacao enviadas para o AS
int AS_waiting_answer = 0;
int udp_fd, tcp_fd, newfd_tcp, errcode;
ssize_t n, n_udp;
socklen_t addrlen_udp;
socklen_t addrlen_tcp;
struct addrinfo hints_tcp, *res_tcp;
struct addrinfo hints_udp, *res_udp;
struct sockaddr_in addr_tcp;
struct sockaddr_in addr_udp;
char buffer[128], tcp_buffer[128];

int pos_atual = 0;
//Estrutura para guardar os dados dos utilizadores que fazem requests
typedef struct{
    char uid[6];
    char tid[6];
    char op[6];
    int fd_tcp;
}tcp_FD;
tcp_FD TCP_FDS[200];

int getFD_TCP(char UID[], char TID[]){
	int right_tcp;
	for (int i = 0; i < pos_atual; i++){
		if ((strcmp(UID, TCP_FDS[i].uid) == 0) && (strcmp(TID, TCP_FDS[i].tid) == 0))
			right_tcp = TCP_FDS[i].fd_tcp;
	}
	return right_tcp;
}

void setFD_TCP(char UID[], char TID[], char OP[], int fd){
	strcpy(TCP_FDS[pos_atual].uid, UID);
	strcpy(TCP_FDS[pos_atual].tid, TID);
	strcpy(TCP_FDS[pos_atual].op, OP);
	TCP_FDS[pos_atual].fd_tcp = fd;
	pos_atual++;
	return;
}



int main(int argc, char *argv[]){
	fd_set active_fd_set, temp_fd_set;
	FD_ZERO(&active_fd_set);
	FD_ZERO(&temp_fd_set);
	int maxfd, retval;
	char portFS[8] = PORT_FS_DEFAULT;
	char portAS[8] = PORT_AS_DEFAULT;
	char ipAS[18] = IP_AS_DEFAULT;
	int fd_tcp_atual;


	for (int i = 1; i <= argc - 1; i++){
		if (argv[i][0] == '-' && strlen(argv[i]) == 2){
			switch (argv[i][1]){
				case 'q':
					strcpy(portFS, argv[i+1]);
					break;
				case 'n':
					strcpy(ipAS, argv[i+1]);
					break;
				case 'p':
					strcpy(portAS, argv[i+1]);
					break;
				case 'v':
					verbose_mode = 1;
					break;
				default: /* invalid flag */
					fprintf(stderr, "Invalid flag\n");
					fprintf(stderr, "Usage: ./FS [-q FSport] [-n ASIP] [-p ASport] [-v]\n");
					exit(1);
			}
		}
	}

	// Create the udp client
	if((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		printf("Error: unable to create udp client\n");
		printf("Error code: %d\n", errno);
		exit(1);
	} 

	memset(&hints_udp, 0, sizeof hints_udp);
	hints_udp.ai_family = AF_INET;
	hints_udp.ai_socktype = SOCK_DGRAM;
		
	errcode = getaddrinfo(ipAS, portAS, &hints_udp, &res_udp);
	if(errcode!=0){
		fprintf(stderr, "Error: Unable to get address info\n");
		fprintf(stderr, "Error code: %d\n", errno);
		exit(1);
	}



	// Create the tcp server
	if((tcp_fd = socket(AF_INET, SOCK_STREAM,0)) == -1){
		printf("Error: unable to create TCP server socket\n");
		printf("Error code: %d\n", errno);
		exit(1);
	}
	
	memset(&hints_tcp, 0, sizeof hints_tcp);
	hints_tcp.ai_family = AF_INET;
	hints_tcp.ai_socktype = SOCK_STREAM;
	hints_tcp.ai_flags = AI_PASSIVE;
		     
	errcode = getaddrinfo(NULL, portFS, &hints_tcp, &res_tcp);
	if(errcode == -1) {  
  		printf("Error: Unable to getaddrinfo \n");
  		printf("Error code: %d\n", errno);
  		exit(1);
  		}
		
	n = bind(tcp_fd,res_tcp->ai_addr, res_tcp->ai_addrlen);
	if(n==-1) {  
  		printf("Error: unable to bind the tcp server socket\n");
  		printf("Error code: %d\n", errno);
  		exit(1);
  		}
	if(listen(tcp_fd, 10) == -1){
		printf("Error: unable to set the server to listen\n");
  		printf("Error code: %d\n", errno);
  		exit(1);
	}
	FD_SET(tcp_fd, &active_fd_set);
	maxfd = tcp_fd;

	while(1){
		char msg[128]="";
		char op[5]="";
		char CNF[5]="";
		char UID[6]="";
		char TID[6]="";
		char FileName[26]="";
		char FileSize[10]="";
		char Data[32]="";

		temp_fd_set = active_fd_set;

		retval = select(maxfd+1,&temp_fd_set,(fd_set *)NULL,(fd_set *)NULL,(struct timeval *) NULL);
		if(retval == -1){
			printf("Error: Select\n");
			exit(1);
		}
		for (int i = 0; i <= maxfd; i++){
			if(!retval)
				break;

			if(FD_ISSET(i, &temp_fd_set)){
				if(i == tcp_fd){
					addrlen_tcp = sizeof(addr_tcp);
					if((newfd_tcp = accept(tcp_fd, (struct sockaddr*) &addr_tcp, &addrlen_tcp)) == -1){
						printf("%s %s\n","error server-accept:", strerror(errno));
						close(tcp_fd);
						exit(1);
					}
					//adiciona a nova conexao ao set ativo. Nao vai ser colocado na verificacao que esta a acontercer atualmente.
					FD_SET(newfd_tcp, &active_fd_set);
					//mantem registo do maximo
					if(newfd_tcp > maxfd)
						maxfd = newfd_tcp;
					retval--;
				}
				else if(i == udp_fd){
					addrlen_udp = sizeof(addr_udp);
					//recebe mensagem do AS
					n_udp = recvfrom(udp_fd, buffer, 128, 0, (struct sockaddr*)&addr_udp, &addrlen_udp);
					if (n_udp == -1){
						printf("Error: unable to receive message\n");
						exit(1);
					}
					write(1, buffer, n_udp);
					// "*" ignora a primeira parte do buffer (CNF)
					sscanf(buffer,"%s %s %s %s %s", CNF, UID, TID, op , FileName);  
					fd_tcp_atual = getFD_TCP(UID, TID);
					if (strcmp(CNF, "CNF") != 0)
						//print ERR(resposta para o User)
					else if (strcmp(op, "E") == 0){
						printf("Error: TID: %s not valid for UID: %s \n", TID, UID);
					}
					else if (strcmp(op, "L") == 0){
						//Listar todos os ficheiros que o respetivo utilizador deu upload anteriormente
						//Utilizar ListDir dado no pdf de apoio ao projeto
						//if no files available > reply: RLS EOF
				
					}
					else if (strcmp(op, "R") == 0){
						if (strcmp(FileName, "") == 0)
							//print ERR(resposta para o User)
						else{
							//Recupera o conteudo do ficheiro "FileName"
						}
					}
					else if (strcmp(op, "U") == 0){
						if (strcmp(FileName, "") == 0)
							//print ERR(resposta para o User)
						else{
							//Upload do conteudo (data) do ficheiro com o nome "FileName" e tamanho "FileSize"
						}	
					}
					else if (strcmp(op, "D") == 0){
						if (strcmp(FileName, "") == 0)
							//print ERR(resposta para o User)
						else{	
							//Apaga o ficheiro com o nome "FileName"
						}
					}
					else if (strcmp(op, "X") == 0){
						//Apaga a informacao do utilizador no AS e depois remove todos os ficheiros e pastas do utilizador no FS
					}
					else{
						//print ERR(resposta para o User)
					}

					AS_waiting_answer--;
					if (AS_waiting_answer == 0)
						FD_CLR(udp_fd, &active_fd_set);
					retval--;
					FD_CLR(fd_tcp_atual, &active_fd_set);
					close(fd_tcp_atual);
				}
				else{
					n = read(i, tcp_buffer, sizeof(tcp_buffer));
					if(n == -1){
						printf("Error: unable to read\n");
						exit(1);
					}

					//Os ultimos 3 argumentos sao opcionais
					sscanf(tcp_buffer,"%s %s %s %*s %s %s", op, UID, TID, FileSize, Data);
					strcat(msg, "VLD ");
					strcat(msg, UID);
					strcat(msg, " ");
					strcat(msg, TID);
					strcat(msg, "\n");

					setFD_TCP(UID, TID, op, i);
					//Manda mensagem ao AS para validar a transacao
					n_udp = sendto(udp_fd, msg, strlen(msg), 0, res_udp->ai_addr, res_udp->ai_addrlen);
					FD_SET(udp_fd, &active_fd_set);
					AS_waiting_answer++;
					if(udp_fd > maxfd)
						maxfd = udp_fd;

					retval--;

				}

			}
		}
		freeaddrinfo(res_udp);
		freeaddrinfo(res_tcp);
		close(udp_fd);
		close(tcp_fd);
	}	

}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <dirent.h>
#include <msg.h>

#define IP_AS_DEFAULT "127.0.0.1"
#define PORT_AS_DEFAULT "58011"
#define PORT_FS_DEFAULT "59011"
#define UNX_ERR 1
#define REQ_ERR 2
#define DIR_ERR 3
#define FILE_ERR 4
#define DUP_ERR 5
#define FULL_ERR 6
#define AS_ERR 7
#define DIR_PATH_INIT "USERS/"

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
    char reply[6];
    char filename[30];
    int fd_tcp;
}tcp_FD;
tcp_FD TCP_FDS[200];

int getFD_TCP(char UID[], char TID[]){
	int right_tcp;
	for (int i = 0; i < pos_atual; i++){
		if ((strcmp(UID, TCP_FDS[i].uid) == 0) && (strcmp(TID, TCP_FDS[i].tid) == 0))
			return i;
	}
}

void setFD_TCP(char UID[], char TID[], char OP[], char Reply[], char FileName[], int fd){
	strcpy(TCP_FDS[pos_atual].uid, UID);
	strcpy(TCP_FDS[pos_atual].tid, TID);
	strcpy(TCP_FDS[pos_atual].op, OP);
	strcpy(TCP_FDS[pos_atual].reply, Reply);
	strcpy(TCP_FDS[pos_atual].filename, FileName);
	TCP_FDS[pos_atual].fd_tcp = fd;
	pos_atual++;
	return;
}
void setFD_TCP1(char UID[], char TID[], char OP[], char Reply[], int fd){
	strcpy(TCP_FDS[pos_atual].uid, UID);
	strcpy(TCP_FDS[pos_atual].tid, TID);
	strcpy(TCP_FDS[pos_atual].op, OP);
	strcpy(TCP_FDS[pos_atual].reply, Reply);
	TCP_FDS[pos_atual].fd_tcp = fd;
	pos_atual++;
	return;
}

//Directory Delete function
void DeleteDirectory(char *dirname){
	DIR *d;
   	struct dirent *dir;
   	d = opendir(dirname);
   	if(d){
		while(dir = readdir(d)){
        	if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")){
            	// do nothing
  			}
        	else
         		remove(dir->d_name);
      	}
   }
   closedir(d);
   remove(dirname);
}

// Directory Listing function 
int ListDir(char *dirname){
	int files = 0;
   	DIR *d;
   	struct dirent *dir;
   	d = opendir(dirname);
   	if(d){
		while(dir = readdir(d)){
        	if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")){
            	// do nothing
  			}
        	else
         		files++;
      	}
   }
   else if (errno == ENOENT){
		printf("Unable to create directory - Does not exist\n");
		exit(1);
	}
	else{
		printf("Unable to create directory\n");
		exit(1);
	}
	closedir(d);
	return files;
}
int Number_of_files(char *dirname){
	int files = 0;
	DIR *d;
	struct dirent *dir;
	d = opendir(dirname);
	if(d){
		while(dir = readdir(d)){
        	if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")){
            	// do nothing
  			}
        	else
         		files++;
      	}
   }
   else if (errno == ENOENT){
		printf("Unable to create directory - Does not exist\n");
		exit(1);
	}
	else{
		printf("Unable to create directory\n");
		exit(1);
	}
	closedir(d);
	return files;
}



int main(int argc, char *argv[]){
	fd_set active_fd_set, temp_fd_set;
	FD_ZERO(&active_fd_set);
	FD_ZERO(&temp_fd_set);
	int maxfd, retval;
	char portFS[8] = PORT_FS_DEFAULT;
	char portAS[8] = PORT_AS_DEFAULT;
	char ipAS[18] = IP_AS_DEFAULT;
	int user_atual, ERR, num_tokens;
	char DIR_PATH[40] = DIR_PATH_INIT;
	DIR *d;
	FILE *fp;
	struct dirent *dir;
	char temp[40];
	char reply_msg[15];
   	char *token;
   	char msg[128] = "";
	char op[5] = "";
	char CNF[5] = "";
	char UID[6] = "";
	char TID[6] = "";
	char FileName[26] = "";
	char FileSize[10] = "";
	char Data[32] = "";


	int check = mkdir("USERS", 0777);
	if (!check)
		printf("Directory Users created\n");
	else if (errno == EEXIST){
		printf("Unable to create directory Users - Already exists\n");
	}
	else{
		printf("Unable to create directory\n");
		exit(1);
	}


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

	memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    signal(SIGPIPE, SIG_IGN);
	while(1){
		temp_fd_set = active_fd_set;

		retval = select(maxfd+1,&temp_fd_set,(fd_set *)NULL,(fd_set *)NULL,(struct timeval *) NULL);
		if(retval == -1){
			printf("Error: Select: %d\n", errno);
			exit(1);
		}
		for (int i = 0; i <= maxfd; i++){
			memset(temp, 0, sizeof(temp));
			memset(msg, 0, sizeof(msg));
			memset(op, 0, sizeof(op));
			memset(CNF, 0, sizeof(CNF));
			memset(UID, 0, sizeof(UID));
			memset(TID, 0, sizeof(TID));
			memset(FileName, 0, sizeof(FileName));
			memset(FileSize, 0, sizeof(FileSize));
			memset(Data, 0, sizeof(Data));
			strcpy(DIR_PATH, DIR_PATH_INIT);
			ERR = 0;
			num_tokens = 0;
			if(!retval)
				break;

			if(FD_ISSET(i, &temp_fd_set)){
				// Recebe conexao nova (NOVO USER)
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
					n_udp = recvfrom(udp_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr_udp, &addrlen_udp);
					if (n_udp == -1){
						printf("Error: unable to receive message\n");
						exit(1);
					}
					sscanf(buffer,"%s %s %s %s %s", CNF, UID, TID, op , FileName);
					strtok(buffer, " ");
					while (token != NULL){
         				num_tokens++;
         				token = strtok(NULL, " ");
      				}
					user_atual = getFD_TCP(UID, TID);

					strcat(DIR_PATH, TCP_FDS[user_atual].uid);
					//LIST
					if (strcmp(op, "E") == 0)
						ERR = AS_ERR;
					else if (strcmp(CNF, "CNF") != 0)
						ERR = UNX_ERR;
					else if (strcmp(op, TCP_FDS[user_atual].op) != 0)
						ERR = AS_ERR;
					else if ((strcmp(op, "L") == 0) && (num_tokens == 4)){
						//Listar todos os ficheiros que o respetivo utilizador deu upload anteriormente
						//list files
						printf("oal\n");
				
					}
					else if ((strcmp(op, "R") == 0) && (num_tokens == 5)){
						if(strcmp(FileName, TCP_FDS[user_atual].filename) != 0)
							ERR = AS_ERR;
						//Recupera o conteudo do ficheiro "FileName"
						else{
							printf("ola\n");
						}
					}
					else if ((strcmp(op, "U") == 0) && (num_tokens == 5)){
						//Upload do conteudo (data) do ficheiro com o nome "FileName" e tamanho "FileSize"
						if(strcmp(FileName, TCP_FDS[user_atual].filename) != 0)
							ERR = AS_ERR;
						else{
							// Verifica se a pasta e temporaria (primeiro upload)
							strcpy(temp, DIR_PATH);
							strcat(DIR_PATH, "_TMP");
							d = opendir(DIR_PATH);
							if (d){
								if(rename(DIR_PATH, temp) == 0)
									printf("RENAMED DIRECTORY\n");
								else
									printf("Unable to rename\n");
								strcpy(DIR_PATH, temp);
								strcat(DIR_PATH, "/");
								strcat(DIR_PATH, TCP_FDS[user_atual].filename);
								strcat(DIR_PATH, "_TMP");
								strcat(temp, "/");
								strcat(temp, TCP_FDS[user_atual].filename);
								if(rename(DIR_PATH, temp) == 0)
									printf("RENAMED file\n");
								else
									printf("Unable to rename file\n");
							}
							//nao existe directoria temporaria
							else if(errno == ENOENT){
								strcpy(DIR_PATH, temp);
								strcat(DIR_PATH, "/");
								strcat(DIR_PATH, TCP_FDS[user_atual].filename);
								strcat(DIR_PATH, "_TMP");
								strcat(temp, "/");
								strcat(temp, TCP_FDS[user_atual].filename);
								if(rename(DIR_PATH, temp) == 0)
									printf("RENAMED file\n");
								else
									printf("Unable to rename file\n");
							}
							else{
								printf("opendir failed\n");
								printf("ERROR %d\n", errno);	
							}
							closedir(d);
						}	
					}
					else if ((strcmp(op, "D") == 0) && (num_tokens == 5)){
						//Apaga o ficheiro com o nome "FileName"
						if(strcmp(FileName, TCP_FDS[user_atual].filename) != 0)
							ERR = AS_ERR;
						else{
							strcat(DIR_PATH, "/");
							strcat(DIR_PATH, FileName);
							if(remove(DIR_PATH) == 0)
								printf("FILE DELETED\n");
							else
								printf("Unable to delete file\n");
						}
					}
					else if ((strcmp(op, "X") == 0) && (num_tokens == 4)){
						//Apaga a informacao do utilizador no AS e depois remove todos os ficheiros e pastas do utilizador no FS
						DeleteDirectory(DIR_PATH);
					}
					//UNEXPECTED ERROR
					else
						ERR = UNX_ERR;
					if (ERR){
						//APAGA O FICHEIRO TEMPORARIO CRIADO NO FS E A DIRECTORIA DO UID SE FOI O PRIMEIRO FICHEIRO
						if(strcmp(TCP_FDS[user_atual].op, "U") == 0){
							strcat(DIR_PATH, "/");
							strcat(DIR_PATH, TCP_FDS[user_atual].filename);
							strcat(DIR_PATH, "_TMP");
							if(remove(DIR_PATH) == 0)
								printf("FILE DELETED\n");
							else
								printf("Unable to delete file\n");
							strcpy(DIR_PATH, DIR_PATH_INIT);
							strcat(DIR_PATH, TCP_FDS[user_atual].uid);
							strcat(DIR_PATH, "_TMP");
							d = opendir(DIR_PATH);
							//EXISTE, LOGO APAGAMOS
							if (d){
								if(remove(DIR_PATH) == 0)
									printf("DIRECTORY DELETED\n");
								else
									printf("Unable to delete directory\n");
							}
							closedir(d);

						}
						if(ERR == UNX_ERR)
							strcpy(msg, "ERR");
						else if(ERR == AS_ERR){
							strcpy(msg, TCP_FDS[user_atual].reply);
							strcat(msg, " INV");
						}
					}

					n = write_buf_SIGPIPE(TCP_FDS[user_atual].fd_tcp, msg);
					if(n == -1){
      						//Conexao ja estava terminada
    						printf("User connection closed(sigpipe).\n");
      				}
					AS_waiting_answer--;
					if (AS_waiting_answer == 0)
						FD_CLR(udp_fd, &active_fd_set);
					retval--;
					FD_CLR(TCP_FDS[user_atual].fd_tcp, &active_fd_set);
					close(TCP_FDS[user_atual].fd_tcp);
				}
				// recebe mensagem de user
				else{
					memset(reply_msg, 0, sizeof(reply_msg));
					//nao esta corrento while( numero maximo de caract ver discord)
					n = read_buf(i, tcp_buffer, sizeof(tcp_buffer));
					if(n == -1){
						printf("Error: unable to read\n");
						fprintf(stderr, "Error code: %d\n", errno);
						exit(1);
					}

					sscanf(tcp_buffer,"%s %s %s %s %s %s", op, UID, TID, FileName, FileSize, Data);
					//Mensagem possivel para o AS - pode nao chegar a ser mandada se houver algum problema com o pedido
					strcat(msg, "VLD ");
					strcat(msg, UID);
					strcat(msg, " ");
					strcat(msg, TID);
					strcat(msg, "\n");
					strtok(tcp_buffer, " ");
					while (token != NULL){
         				num_tokens++;
         				token = strtok(NULL, " ");
      				}
      				if((strcmp(op, "LST") == 0)){
      					//Mensagem possivel logo para o User
      					strcpy(reply_msg, "RLS");
      					//verifica se o pedido foi bem formulado
      					if((num_tokens == 3) && (strlen(UID) == 5) && (strlen(TID) == 4)){
      						strcat(DIR_PATH, UID);
      						if(!(Number_of_files(DIR_PATH))){
      							printf("Nao existem ficheiros nesta diretoria\n");
      							ERR = FILE_ERR;
      						}
      						else
      							setFD_TCP1(UID, TID, "L", reply_msg, i);
      					}
      					//senao, houve erro na formulacao do pedido "RLS ERR"
      					else
      						ERR = REQ_ERR;
      				}
      				else if((strcmp(op, "DEL") == 0)){
      					memset(temp, 0, sizeof(temp));
      					strcpy(reply_msg, "RDL");
						if((num_tokens == 4) && (strlen(UID) == 5) && (strlen(TID) == 4)){
							// Agora vai verificar se o UID tem conteudos no FS
							strcat(DIR_PATH, UID);
							d = opendir(DIR_PATH);
							strcpy(temp, DIR_PATH);
							strcat(temp, "/");
							strcat(temp, FileName);
							// 'ENOENT' significa que directoria nao existe -> NOK
							if (errno == ENOENT){
								printf("Directory does not exist\n");
								ERR = DIR_ERR;
							}
							//Directoria nao tem ficheiros(situacao em que se faz upload de um ficheiro e depois remove-se o mesmo) -> NOK
							else if(Number_of_files(DIR_PATH) == 0){
								printf("UID %s nao tem conteudo no FS\n", UID);
								ERR = DIR_ERR;
							}
							// Ficheiro nao existe -> EOF
							else if((fp = fopen(temp, "r")) == NULL){
								printf("Ficheiro nao existe na directoria do UID %s\n", UID);
								ERR = FILE_ERR;
							}
							// Esta tudo bem por agora (nao houve erro nenhum), ou seja, vai guarda na estrutura de dados e vai pedir validacao ao AS
							else
								setFD_TCP(UID, TID, "D", reply_msg, FileName, i);
							fclose(fp);
							closedir(d);						
						}
						else
							ERR = REQ_ERR;
					}

      				else if((strcmp(op, "REM") == 0)){
      					strcpy(reply_msg, "RRM");
						if((num_tokens == 3) && (strlen(UID) == 5) && (strlen(TID) == 4)){
							// Agora vai verificar se o UID tem conteudos no FS
							strcat(DIR_PATH, UID);
							d = opendir(DIR_PATH);
							// 'ENOENT' significa que directoria nao existe -> NOK
							if (errno == ENOENT){
								printf("directory does not exist\n");
								ERR = DIR_ERR;
							}
							else
								setFD_TCP1(UID, TID, "X", reply_msg, i);
							closedir(d);
						}
						else
							ERR = REQ_ERR;
      				}
      				else if((strcmp(op, "RTV") == 0)){
      					strcpy(reply_msg, "RRT");
      					if((num_tokens == 4) && (strlen(UID) == 5) && (strlen(TID) == 4)){
      						// Agora vai verificar se o UID tem conteudos no FS
							strcat(DIR_PATH, UID);
							d = opendir(DIR_PATH);
							strcat(DIR_PATH, "/");
							strcat(DIR_PATH, FileName);
							// 'ENOENT' significa que directoria nao existe -> NOK
							if (errno == ENOENT){
								printf("Directory does not exist\n");
								ERR = DIR_ERR;
							}
							//Directoria nao tem ficheiros(situacao em que se faz upload de um ficheiro e depois remove-se o mesmo) -> NOK
							else if(Number_of_files(DIR_PATH) == 0){
								printf("UID %s nao tem conteudo no FS\n", UID);
								ERR = DIR_ERR;
							}
							// Ficheiro nao existe -> EOF
							else if((fp = fopen(DIR_PATH, "r")) == NULL){
								printf("Ficheiro nao existe na directoria do UID %s\n", UID);
								ERR = FILE_ERR;
							}
							else
								setFD_TCP(UID, TID, "R", reply_msg, FileName, i);
							fclose(fp);
							closedir(d);
      					}
      					else
      						ERR = REQ_ERR;
      				}
      				//IMC
      				else if((strcmp(op, "UPL") == 0)){
      					strcpy(reply_msg, "RUP");
						if((num_tokens > 5) && (strlen(UID) == 5) && (strlen(TID) == 4)){
      						//INC
      						strcat(DIR_PATH, UID);
      						d = opendir(DIR_PATH);
      						if (d){
      							int n_files = Number_of_files(DIR_PATH);
      							strcat(DIR_PATH, "/");
								strcat(DIR_PATH, FileName);
								//significa que ja existe
								if ((fp = fopen(DIR_PATH, "r"))){
									printf("Ficheiro ja existe\n");
									ERR = DUP_ERR;
									fclose(fp);
								}
								else if (n_files == 15){
									printf("User ja tem 15 files. Nao pode fazer upload\n");
									ERR = FULL_ERR;
								}
								//cria file temporario
								else{
									strcat(DIR_PATH,"_TMP");
									fp = fopen(DIR_PATH, "a");
									//cria file temp com o conteudo
								}
      						}
      						else if(errno == ENOENT){
      							//cria directory temp e file temp
      							strcat(DIR_PATH, "_TMP");
      							int check = mkdir(DIR_PATH, 0777);
								if (!check)
									printf("Directory Users created\n");
								else{
									printf("Unable to create directory\n");
									exit(1);
								}
								//cria fich temp com o conteudo
      						}
      						closedir(d);
      						setFD_TCP(UID, TID, "U", reply_msg, FileName, i);
      					}
      					//FILESIZE?
      					else
      						ERR = REQ_ERR;
      				}
      				// Unexpected protocol message
      				else
      					ERR = UNX_ERR;

      				// verifica se houve algum erro
      				if (ERR){
      					if(ERR == UNX_ERR)
      						strcpy(reply_msg, "ERR");
      					else if(ERR == REQ_ERR)
							strcat(reply_msg, " ERR");
						else if(ERR == DIR_ERR)
							strcat(reply_msg, " NOK");
						else if(ERR == FILE_ERR)
							strcat(reply_msg, " EOF");
						else if(ERR == DUP_ERR)
							strcat(reply_msg, " DUP");
						else if(ERR == FULL_ERR)
							strcat(reply_msg, " FULL");
      					n = write_buf_SIGPIPE(i, reply_msg);
      					if(n == -1){
      						//Conexao ja estava terminada
    						printf("User connection closed(sigpipe).\n");
      					}
      					retval--;
      					FD_CLR(i, &active_fd_set);
						close(i);
						continue;
      				}
					// Se nao houve erro, manda mensagem ao AS para validar a transacao
					n_udp = sendto(udp_fd, msg, strlen(msg), 0, res_udp->ai_addr, res_udp->ai_addrlen);
					FD_SET(udp_fd, &active_fd_set);
					AS_waiting_answer++;
					if(udp_fd > maxfd)
						maxfd = udp_fd;
					retval--;
				}
			}
		}
	}

	freeaddrinfo(res_udp);
	freeaddrinfo(res_tcp);
	close(udp_fd);
	close(tcp_fd);
}
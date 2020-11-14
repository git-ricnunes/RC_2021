#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "tcpFiles.h"
#include "msg.h"
#include "udpTimeout.h"

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
#define TIMEOUT_DEFAULT 20
#define MAX_BYTES 51
#define BUFFER_SIZE 1024
#define UID_SIZE 6
#define TID_SIZE 5
#define F_NAME_SIZE 25
#define F_SIZE 11

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
char buffer[BUFFER_SIZE], tcp_buffer[BUFFER_SIZE];
struct sigaction act;
struct timeval tv;
char ipUser[30];
int portUser;

int pos_atual = 0;
//Estrutura para guardar os dados dos utilizadores que fazem requests
typedef struct {
    char uid[UID_SIZE];
    char tid[TID_SIZE];
    char op[6];
    char reply[6];
    char filename[F_NAME_SIZE];
    int fd_tcp;
    int used;
} tcp_FD;
tcp_FD TCP_FDS[600];

int getFD_TCP(char UID[], char TID[]) {
    for (int i = 0; i < pos_atual; i++) {
        if ((strcmp(UID, TCP_FDS[i].uid) == 0) && (strcmp(TID, TCP_FDS[i].tid) == 0) && (TCP_FDS[i].used == 0))
            return i;
    }
}


void foo(int fd){
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(fd, (struct sockaddr *)&addr, &addr_size);
    strcpy(ipUser, inet_ntoa(addr.sin_addr));
    portUser = ntohs(addr.sin_port);
}
void setFD_TCP(char *UID, char *TID, char *OP, char *Reply, char *FileName, int fd) {
    strcpy(TCP_FDS[pos_atual].uid, UID);
    strcpy(TCP_FDS[pos_atual].tid, TID);
    strcpy(TCP_FDS[pos_atual].op, OP);
    strcpy(TCP_FDS[pos_atual].reply, Reply);
    strcpy(TCP_FDS[pos_atual].filename, FileName);
    TCP_FDS[pos_atual].fd_tcp = fd;
    TCP_FDS[pos_atual].used = 0;
    pos_atual++;
    return;
}
void setFD_TCP1(char *UID, char *TID, char *OP, char *Reply, int fd) {
    strcpy(TCP_FDS[pos_atual].uid, UID);
    strcpy(TCP_FDS[pos_atual].tid, TID);
    strcpy(TCP_FDS[pos_atual].op, OP);
    strcpy(TCP_FDS[pos_atual].reply, Reply);
    TCP_FDS[pos_atual].fd_tcp = fd;
    TCP_FDS[pos_atual].used = 0;
    pos_atual++;
    return;
}

//Directory Delete function
void DeleteDirectory(char *dirname) {
    DIR *d;
    int n;
    char deleted_file[50];
    struct dirent *dir;
    d = opendir(dirname);
    if (d) {
        while (dir = readdir(d)) {
            memset(deleted_file, 0, sizeof(deleted_file));
            strcpy(deleted_file, dirname);
            strcat(deleted_file, "/");
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") || !strncmp(dir->d_name, ".", 1)) {
                // do nothing
            } else{
                strcat(deleted_file, dir->d_name);
                n = remove(deleted_file);
                if (n == -1){
                    printf("%d\n", errno);
                }
            }
        }
    }
    closedir(d);
    if(remove(dirname) == 0){
        printf("pasta apagada\n");
    }
    else{
        printf("%d\n", errno);
    }

    return;
}

int Number_of_files(char *dirname) {
    int files = 0;
    DIR *d;
    struct dirent *dir;
    d = opendir(dirname);
    if (d) {
        while (dir = readdir(d)) {
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") || !strncmp(dir->d_name, ".", 1)) {
                // do nothing
            } else{
                files++;
            }
        }
    } else if (errno == ENOENT) {
        printf("Directory does not exist\n");
        return 0;
    } else {
        printf("ERROR: %d\n", errno);
        exit(1);
    }
    closedir(d);
    return files;
}
int checkSizeFile(char *filename){
	long res;
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) { 
        printf("File Not Found!\n"); 
        exit(1);
    }
    fseek(fp, 0L, SEEK_END);
    res = ftell(fp);
    if (fp != NULL)
        fclose(fp);
    return res;

}

int RetrieveFile(char *filename, int fd){
	char msg[10] = "";
	int n_sent;
	strcpy(msg, "RRT OK ");
	n_sent = write_buf_SIGPIPE(fd, msg, strlen(msg));
	if (n_sent == -1)
		return n_sent;
	send_file(fd, filename, SP_CHECK);
    return 0;
}

// Directory Listing function
int ListDir(char *dirname, int fd) {
	char msg[100] = "";
    DIR *d;
    int n_atual = 0;
    struct dirent *dir;
	char n_files[4];
	char file_size[F_SIZE];
	char temp[50];
	int nfiles;
    long filesize;
	int n_sent;
	strcpy(msg, "RLS ");
    nfiles = Number_of_files(dirname);
    sprintf(n_files, "%d ", nfiles);
    strcat(msg, n_files);
    strcpy(temp, dirname);
    d = opendir(dirname);
    if (d) {
        while (dir = readdir(d)) {
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") || !strncmp(dir->d_name, ".", 1)) {
                // do nothing
            }
            else{
                n_atual++;
            	strcat(temp, "/");
            	strcat(temp, dir->d_name);
            	filesize = checkSizeFile(temp);
            	strcat(msg, dir->d_name);
            	strcat(msg, " ");
            	sprintf(file_size, "%ld", filesize);
            	strcat(msg, file_size);
                if(n_atual < nfiles)
            	   strcat(msg, " ");
            	n_sent = write_buf_SIGPIPE(fd, msg, strlen(msg));
            	if (n_sent == -1)
					return n_sent;
            	memset(msg, 0, sizeof(msg));
            	strcpy(temp, dirname);
            }    
        }
        sprintf(msg, "\n");
        n_sent = write_buf_SIGPIPE(fd, msg, strlen(msg));
        if (verbose_mode){
            printf("Message sent to User\n");
        }
    } 
    else if (errno == ENOENT) {
        printf("Directory does not exist\n");
        exit(1);
    } else {
        printf("Error: %d\n", errno);
        exit(1);
    }
    closedir(d);
    return 0;
}


int main(int argc, char *argv[]) {
    fd_set active_fd_set, temp_fd_set;
    FD_ZERO(&active_fd_set);
    FD_ZERO(&temp_fd_set);
    int maxfd, retval;
    char portFS[8] = PORT_FS_DEFAULT;
    char portAS[8] = PORT_AS_DEFAULT;
    char ipAS[18] = IP_AS_DEFAULT;
    int user_atual, ERR, num_tokens;
    int bytes_data;
    int n_files;
    char DIR_PATH[40] = DIR_PATH_INIT;
    DIR *d;
    FILE *fp;
    struct dirent *dir;
    char temp[40];
    char reply_msg[15];
    char *token;
    char msg[128];
    char op[6];
    char CNF[6];
    char UID[UID_SIZE];
    char TID[TID_SIZE];
    char FileName[F_NAME_SIZE];
    char FileSize[F_SIZE];
    char Data[32];
    srand(time(NULL));
    tv.tv_sec = TIMEOUT_DEFAULT;
    tv.tv_usec = 0;

    int check = mkdir("USERS", 0777);
    if (!check)
        printf("Directory Users created\n");
    else if (errno == EEXIST) {
        printf("Unable to create directory Users - Already exists and problably has old files\n");
    } else {
        printf("Unable to create directory\n");
        exit(1);
    }

    for (int i = 1; i <= argc - 1; i++) {
        if (argv[i][0] == '-' && strlen(argv[i]) == 2) {
            switch (argv[i][1]) {
                case 'q':
                    strcpy(portFS, argv[i + 1]);
                    break;
                case 'n':
                    strcpy(ipAS, argv[i + 1]);
                    break;
                case 'p':
                    strcpy(portAS, argv[i + 1]);
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
    if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        printf("Error: unable to create udp client\n");
        printf("Error code: %d\n", errno);
        exit(1);
    }

    memset(&hints_udp, 0, sizeof hints_udp);
    hints_udp.ai_family = AF_INET;
    hints_udp.ai_socktype = SOCK_DGRAM;

    errcode = getaddrinfo(ipAS, portAS, &hints_udp, &res_udp);
    if (errcode != 0) {
        fprintf(stderr, "Error: Unable to get address info\n");
        fprintf(stderr, "Error code: %d\n", errno);
        exit(1);
    }

    // Create the tcp server
    if ((tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Error: unable to create TCP server socket\n");
        printf("Error code: %d\n", errno);
        exit(1);
    }

    memset(&hints_tcp, 0, sizeof hints_tcp);
    hints_tcp.ai_family = AF_INET;
    hints_tcp.ai_socktype = SOCK_STREAM;
    hints_tcp.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, portFS, &hints_tcp, &res_tcp);
    if (errcode == -1) {
        printf("Error: Unable to getaddrinfo \n");
        printf("Error code: %d\n", errno);
        exit(1);
    }

    n = bind(tcp_fd, res_tcp->ai_addr, res_tcp->ai_addrlen);
    if (n == -1) {
        printf("Error: unable to bind the tcp server socket\n");
        printf("Error code: %d\n", errno);
        exit(1);
    }
    if (listen(tcp_fd, 10) == -1) {
        printf("Error: unable to set the server to listen\n");
        printf("Error code: %d\n", errno);
        exit(1);
    }
    FD_SET(tcp_fd, &active_fd_set);
    maxfd = tcp_fd;

    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    signal(SIGPIPE, SIG_IGN);
    setTimeoutUDP(udp_fd, TIMEOUT_DEFAULT);

    while (1) {
    	FD_ZERO(&temp_fd_set);
        temp_fd_set = active_fd_set;

        retval = select(maxfd + 1, &temp_fd_set, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
        if (retval == -1) {
            printf("Error: Select: %d\n", errno);
            perror(" ERROR");
            exit(1);
        }
        for (int i = 0; i <= maxfd; i++) {
            memset(temp, 0, sizeof(temp));
            memset(tcp_buffer, 0, sizeof(tcp_buffer));
            memset(buffer, 0, sizeof(buffer));
            memset(msg, 0, sizeof(msg));
            memset(op, 0, sizeof(op));
            memset(CNF, 0, sizeof(CNF));
            memset(UID, 0, sizeof(UID));
            memset(TID, 0, sizeof(TID));
            memset(FileName, 0, sizeof(FileName));
            memset(FileSize, 0, sizeof(FileSize));
            memset(DIR_PATH, 0, sizeof(DIR_PATH));
            memset(Data, 0, sizeof(Data));
            strcpy(DIR_PATH, DIR_PATH_INIT);
            ERR = 0;
            num_tokens = 0;
            if (!retval)
                break;
            if (FD_ISSET(i, &temp_fd_set)) {
                // Recebe conexao nova (NOVO USER)
                if (i == tcp_fd) {
                    addrlen_tcp = sizeof(addr_tcp);
                    if ((newfd_tcp = accept(tcp_fd, (struct sockaddr *)&addr_tcp, &addrlen_tcp)) == -1) {
                        printf("%s %s\n", "error server-accept:", strerror(errno));
                        close(tcp_fd);
                        exit(1);
                    }
                    //adiciona a nova conexao ao set ativo. Nao vai ser colocado na verificacao que esta a acontercer atualmente.
                    FD_SET(newfd_tcp, &active_fd_set);
                    //mantem registo do maximo
                    if (newfd_tcp > maxfd)
                        maxfd = newfd_tcp;
                    retval--;
                } else if (i == udp_fd) {
                    addrlen_udp = sizeof(addr_udp);
                    //recebe mensagem do AS
                    n_udp = recvfrom(udp_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr_udp, &addrlen_udp);
                    if (checkTimeoutUdp(n_udp) == -1)
                        break;
                    if (verbose_mode){
                    	printf("IP: %s Port: %s\nMessage received from AS:\n%s", ipAS, portAS, buffer);
                    }
                    sscanf(buffer, "%s %s %s %s %s", CNF, UID, TID, op, FileName);
                    token = strtok(buffer, " ");
                    while (token != NULL) {
                        num_tokens++;
                        token = strtok(NULL, " ");
                    }
                    user_atual = getFD_TCP(UID, TID);

                    strcat(DIR_PATH, TCP_FDS[user_atual].uid);
                    if (strcmp(op, "E") == 0)
                        ERR = AS_ERR;
                    else if (strcmp(CNF, "CNF") != 0){
                        ERR = UNX_ERR;
                    }
                    else if (strcmp(op, TCP_FDS[user_atual].op) != 0)
                        ERR = AS_ERR;
                    else if ((strcmp(op, "L") == 0) && (num_tokens == 4)) {
                        //Listar todos os ficheiros que o respetivo utilizador deu upload anteriormente
                       n = ListDir(DIR_PATH, TCP_FDS[user_atual].fd_tcp);
                       if (n == -1) {
                       		//Conexao ja estava terminada
                       		printf("User connection closed(sigpipe).\n");
                   		}
                    }
                    else if ((strcmp(op, "R") == 0) && (num_tokens == 5)) {
                        if (strcmp(FileName, TCP_FDS[user_atual].filename) != 0)
                            ERR = AS_ERR;
                        //Recupera o conteudo do ficheiro "FileName"
                        else {
                        	strcat(DIR_PATH, "/");
                        	strcat(DIR_PATH, FileName);
                        	n = RetrieveFile(DIR_PATH, TCP_FDS[user_atual].fd_tcp);
                        	if (n == -1) {
                       			//Conexao ja estava terminada
                       			printf("User connection closed(sigpipe).\n");
                   			}
                            else{
                                if (verbose_mode){
                                    printf("Message sent to User\n");
                                }
                            }
                        }
                    }
                    else if ((strcmp(op, "U") == 0) && (num_tokens == 5)) {
                        //Upload do conteudo (data) do ficheiro com o nome "FileName" e tamanho "FileSize"
                        if (strcmp(FileName, TCP_FDS[user_atual].filename) != 0)
                            ERR = AS_ERR;
                        else {
                            // Verifica se a pasta e temporaria (primeiro upload)
                            strcpy(temp, DIR_PATH);
                            strcat(DIR_PATH, "_TMP");
                            d = opendir(DIR_PATH);
                            if (d) {
                                if (rename(DIR_PATH, temp) == 0)
                                    printf("RENAMED DIRECTORY\n");
                                else{
                                    printf("Unable to rename\n");
                                    printf("Error: %d\n", errno);
                                    exit(1);
                                }
                                strcpy(DIR_PATH, temp);
                                strcat(DIR_PATH, "/");
                                strcat(DIR_PATH, TCP_FDS[user_atual].filename);
                                strcat(DIR_PATH, "_TMP");
                                strcat(temp, "/");
                                strcat(temp, TCP_FDS[user_atual].filename);
                                if (rename(DIR_PATH, temp) == 0){
                                    printf("RENAMED file\n");
                                    if (verbose_mode){
                                        printf("Ficheiro %s guardado na pasta USERS/%s\n", TCP_FDS[user_atual].filename, TCP_FDS[user_atual].uid);
                                    }
                                }
                                else{
                                    printf("Unable to rename file\n");
                                    printf("Error: %d\n", errno);
                                    exit(1);
                                }

                            }
                            //nao existe directoria temporaria
                            else if (errno == ENOENT) {
                                strcpy(DIR_PATH, temp);
                                strcat(DIR_PATH, "/");
                                strcat(DIR_PATH, TCP_FDS[user_atual].filename);
                                strcat(DIR_PATH, "_TMP");
                                strcat(temp, "/");
                                strcat(temp, TCP_FDS[user_atual].filename);
                                if (rename(DIR_PATH, temp) == 0){
                                    printf("RENAMED file\n");
                                    if (verbose_mode){
                                        printf("Ficheiro %s guardado na pasta USERS/%s\n", TCP_FDS[user_atual].filename, TCP_FDS[user_atual].uid);
                                    }
                                }
                                else{
                                    printf("Unable to rename file\n");
                                    printf("Error: %d\n", errno);
                                    exit(1);
                                }

                            } 
                            else {
                                printf("opendir failed\n");
                                printf("ERROR %d\n", errno);
                                exit(1);
                            }
                            strcpy(msg, TCP_FDS[user_atual].reply);
                            strcat(msg, " OK\n");
                            closedir(d);
                        }
                    }
                    else if ((strcmp(op, "D") == 0) && (num_tokens == 5)) {
                        //Apaga o ficheiro com o nome "FileName"
                        if (strcmp(FileName, TCP_FDS[user_atual].filename) != 0)
                            ERR = AS_ERR;
                        else {
                            strcat(DIR_PATH, "/");
                            strcat(DIR_PATH, FileName);
                            if (remove(DIR_PATH) == 0)
                                printf("FILE DELETED\n");
                            else{
                                printf("Unable to delete file\n");
                                printf("Error: %d\n", errno);
                                exit(1);
                            }
                            strcpy(msg, TCP_FDS[user_atual].reply);
                            strcat(msg, " OK\n");
                            if(verbose_mode)
                                printf("Ficheiro %s apagado\n", TCP_FDS[user_atual].filename);
                        }
                    } else if ((strcmp(op, "X") == 0) && (num_tokens == 4)) {
                        //Apaga a informacao do utilizador no AS e depois remove todos os ficheiros e pastas do utilizador no FS
                        DeleteDirectory(DIR_PATH);
                        strcpy(msg, TCP_FDS[user_atual].reply);
                        strcat(msg, " OK\n");
                        if(verbose_mode)
                            printf("Ficheiros e pasta do User %s apagados\n", TCP_FDS[user_atual].uid);
                    }
                    //UNEXPECTED ERROR
                    else
                        ERR = UNX_ERR;
                    if (ERR > 0) {
                        //APAGA O FICHEIRO TEMPORARIO CRIADO NO FS E A DIRECTORIA DO UID SE FOI O PRIMEIRO FICHEIRO
                        if (strcmp(TCP_FDS[user_atual].op, "U") == 0) {
                            strcpy(DIR_PATH, DIR_PATH_INIT);
                            strcat(DIR_PATH, TCP_FDS[user_atual].uid);
                            strcpy(temp, DIR_PATH);
                            strcat(DIR_PATH, "_TMP");
                            d = opendir(DIR_PATH);
                            if (d) {
                                strcat(DIR_PATH, "/");
                                strcat(DIR_PATH, TCP_FDS[user_atual].filename);
                                strcat(DIR_PATH, "_TMP");
                                if (remove(DIR_PATH) == 0)
                                    printf("TMP FILE DELETED\n");
                                else
                                    printf("Unable to delete tmp file\n");
                                strcpy(DIR_PATH, temp);
                                strcat(DIR_PATH, "_TMP");
                                if (remove(DIR_PATH) == 0)
                                    printf("TMP DIRECTORY DELETED\n");
                                else
                                    printf("Unable to delete tmp directory\n");
                            }
                            //nao existe directory tmp
                            else if (errno == ENOENT){
                                strcpy(DIR_PATH, temp);
                                strcat(DIR_PATH, "/");
                                strcat(DIR_PATH, TCP_FDS[user_atual].filename);
                                strcat(DIR_PATH, "_TMP");
                                if (remove(DIR_PATH) == 0)
                                    printf("TMP FILE DELETED\n");
                                else
                                    printf("Unable to delete tmp file\n");
                            }
                            else{
                                printf("opendir failed\n");
                                printf("ERROR %d\n", errno);
                                exit(1);
                            }
                            closedir(d);
                        }
                        if (ERR == UNX_ERR)
                            strcpy(msg, "ERR\n");
                        else if (ERR == AS_ERR) {
                            strcpy(msg, TCP_FDS[user_atual].reply);
                            strcat(msg, " INV\n");
                        }
                    }
                    if ((ERR > 0) || ((strcmp(op, "L") != 0) && (strcmp(op, "R") != 0))){
                    	n = write_buf_SIGPIPE(TCP_FDS[user_atual].fd_tcp, msg, strlen(msg));
                   		if (n == -1) {
                       		//Conexao ja estava terminada
                       		printf("User connection closed(sigpipe).\n");
                   		}
                        else{
                            if (verbose_mode){
                                printf("Message sent to User\n");
                            }
                        }
                   	}
                    TCP_FDS[user_atual].used = 1;
                    AS_waiting_answer--;
                    if (AS_waiting_answer == 0)
                        FD_CLR(udp_fd, &active_fd_set);
                    retval--;
                    close(TCP_FDS[user_atual].fd_tcp);
                }
                // recebe mensagem de user
                else {
                    memset(reply_msg, 0, sizeof(reply_msg));
                    n = read_buf(i, tcp_buffer, sizeof(tcp_buffer), MAX_BYTES, NULL_IGNORE);
                    foo(i);
                    sscanf(tcp_buffer, "%s %s %s %s %s", op, UID, TID, FileName, FileSize);
                    //Mensagem possivel para o AS - pode nao chegar a ser mandada se houver algum problema com o pedido
                    strcat(msg, "VLD ");
                    strcat(msg, UID);
                    strcat(msg, " ");
                    strcat(msg, TID);
                    strcat(msg, "\n");
                    if (strcmp(op, "UPL") != 0){
                    	if (verbose_mode){
                    		printf("IP: %s Port: %d\nMessage received from User:\n%s", ipUser, portUser, tcp_buffer);
                    	}
                    	token = strtok(tcp_buffer, " ");
                    	while (token != NULL) {
                        	num_tokens++;
                        	token = strtok(NULL, " ");
                    	}
                    }
                    else{
                    	if (verbose_mode){
                    	printf("IP: %s Port: %d\nMensagem recebida do user\n", ipUser, portUser);
                        }
                    }
                    if ((strcmp(op, "LST") == 0)) {
                        //Mensagem possivel logo para o User
                        strcpy(reply_msg, "RLS");
                        //verifica se o pedido foi bem formulado
                        if ((num_tokens == 3) && (strlen(UID) == UID_SIZE - 1) && (strlen(TID) == TID_SIZE - 1)) {
                            strcat(DIR_PATH, UID);
                            if ((Number_of_files(DIR_PATH)) == 0) {
                                printf("Nao existem ficheiros nesta diretoria\n");
                                ERR = FILE_ERR;
                            } else
                                setFD_TCP1(UID, TID, "L", reply_msg, i);
                        }
                        //senao, houve erro na formulacao do pedido "RLS ERR"
                        else
                            ERR = REQ_ERR;
                    } else if ((strcmp(op, "DEL") == 0)) {
                        memset(temp, 0, sizeof(temp));
                        strcpy(reply_msg, "RDL");
                        if ((num_tokens == 4) && (strlen(UID) == UID_SIZE - 1) && (strlen(TID) == TID_SIZE - 1) && (strlen(FileName) <= F_NAME_SIZE - 1)) {
                            // Agora vai verificar se o UID tem conteudos no FS
                            strcat(DIR_PATH, UID);
                            d = opendir(DIR_PATH);
                            strcpy(temp, DIR_PATH);
                            strcat(temp, "/");
                            strcat(temp, FileName);
                            fp = fopen(temp, "r");
                            // 'ENOENT' significa que directoria nao existe -> NOK
                            if (errno == ENOENT) {
                                printf("Directory does not exist\n");
                                ERR = DIR_ERR;
                            }
                            //Directoria nao tem ficheiros(situacao em que se faz upload de um ficheiro e depois remove-se o mesmo) -> NOK
                            else if (Number_of_files(DIR_PATH) == 0) {
                                printf("UID %s nao tem conteudo no FS\n", UID);
                                ERR = DIR_ERR;
                            }
                            // Ficheiro nao existe -> EOF
                            else if (fp == NULL) {
                                printf("Ficheiro nao existe na directoria do UID %s\n", UID);
                                ERR = FILE_ERR;
                            }
                            // Esta tudo bem por agora (nao houve erro nenhum), ou seja, vai guarda na estrutura de dados e vai pedir validacao ao AS
                            else{
                                setFD_TCP(UID, TID, "D", reply_msg, FileName, i);
                            }
                            if(fp != NULL)
                                fclose(fp);
                            closedir(d);
                        } else{
                            ERR = REQ_ERR;
                        }
                    }

                    else if ((strcmp(op, "REM") == 0)) {
                        strcpy(reply_msg, "RRM");
                        if ((num_tokens == 3) && (strlen(UID) == UID_SIZE - 1) && (strlen(TID) == TID_SIZE - 1)) {
                            // Agora vai verificar se o UID tem conteudos no FS
                            strcat(DIR_PATH, UID);
                            d = opendir(DIR_PATH);
                            // 'ENOENT' significa que directoria nao existe -> NOK
                            if (errno == ENOENT) {
                                printf("directory does not exist\n");
                                ERR = DIR_ERR;
                            } else
                                setFD_TCP1(UID, TID, "X", reply_msg, i);
                            closedir(d);
                        } else
                            ERR = REQ_ERR;
                    } else if ((strcmp(op, "RTV") == 0)) {
                        strcpy(reply_msg, "RRT");
                        if ((num_tokens == 4) && (strlen(UID) == UID_SIZE - 1) && (strlen(TID) == TID_SIZE - 1) && (strlen(FileName) <= F_NAME_SIZE - 1)) {
                            // Agora vai verificar se o UID tem conteudos no FS
                            strcat(DIR_PATH, UID);
                            strcpy(temp, DIR_PATH);
                            d = opendir(DIR_PATH);
                            strcat(DIR_PATH, "/");
                            strcat(DIR_PATH, FileName);
                            fp = fopen(DIR_PATH, "r");
                            // 'ENOENT' significa que directoria nao existe -> NOK
                            if (errno == ENOENT) {
                                printf("Directory does not exist\n");
                                ERR = DIR_ERR;
                            }
                            //Directoria nao tem ficheiros(situacao em que se faz upload de um ficheiro e depois remove-se o mesmo) -> NOK
                            else if (Number_of_files(temp) == 0) {
                                printf("UID %s nao tem conteudo no FS\n", UID);
                                ERR = DIR_ERR;
                            }
                            // Ficheiro nao existe -> EOF
                            else if (fp == NULL) {
                                printf("Ficheiro nao existe na directoria do UID %s\n", UID);
                                ERR = FILE_ERR;
                            } else{
                                setFD_TCP(UID, TID, "R", reply_msg, FileName, i);
                            }
                            if (fp != NULL)
                                fclose(fp);
                            closedir(d);
                        } else
                            ERR = REQ_ERR;
                    }
                    else if ((strcmp(op, "UPL") == 0)) {
                        strcpy(reply_msg, "RUP");
                        if ((strlen(UID) == UID_SIZE - 1) && (strlen(TID) == TID_SIZE - 1) && (strlen(FileName) <= F_NAME_SIZE - 1) && (strlen(FileSize) <= F_SIZE - 1)) {
                        	bytes_data = strlen(op) + 1 + strlen(UID) + 1 + strlen(TID) + 1 + strlen(FileName) + 1 + strlen(FileSize) + 1;
                            strcat(DIR_PATH, UID);
                            d = opendir(DIR_PATH);
                            if (d) {
                                n_files = Number_of_files(DIR_PATH);
                                strcat(DIR_PATH, "/");
                                strcat(DIR_PATH, FileName);
                                fp = fopen(DIR_PATH, "r");
                                //significa que ja existe
                                if (fp != NULL) {
                                    printf("Ficheiro ja existe\n");
                                    ERR = DUP_ERR;
                                    fclose(fp);
                                    //SO FAZ READ MAS NAO GUARDA EM LADO NENHUM
                                    recv_file(i, DIR_PATH, atol(FileSize), tcp_buffer + bytes_data, n - bytes_data, F_EXISTS);
                                } else if (n_files == 15) {
                                    printf("User ja tem 15 files. Nao pode fazer upload\n");
                                    ERR = FULL_ERR;
                                    // FAZ READ MAS NAO GUARDA EM LADO NENHUM
                                    recv_file(i, DIR_PATH, atol(FileSize), tcp_buffer + bytes_data, n - bytes_data, F_EXISTS);
                                }
                                //cria file temporario
                                else {
                                    strcat(DIR_PATH, "_TMP");
                                    recv_file(i, DIR_PATH, atol(FileSize), tcp_buffer + bytes_data, n - bytes_data, F_NEXISTS);
                                }
                            } else if (errno == ENOENT) {
                                //cria directory temp e file temp

                                strcat(DIR_PATH, "_TMP");
                                
                                int check = mkdir(DIR_PATH, 0777);
                                if (!check)
                                    printf("Directory Users created\n");
                                else {
                                    printf("Unable to create directory\n");
                                    exit(1);
                                }
                                strcat(DIR_PATH, "/");
                                strcat(DIR_PATH, FileName);
                                strcat(DIR_PATH, "_TMP");
                                recv_file(i, DIR_PATH, atol(FileSize), tcp_buffer + bytes_data, n - bytes_data, F_NEXISTS);
                            }
                            closedir(d);
                            setFD_TCP(UID, TID, "U", reply_msg, FileName, i);
                        }
                        else
                            ERR = REQ_ERR;
                    }
                    // Unexpected protocol message
                    else
                        ERR = UNX_ERR;

                    // verifica se houve algum erro
                    if (ERR > 0) {
                        if (ERR == UNX_ERR)
                            strcpy(reply_msg, "ERR\n");
                        else if (ERR == REQ_ERR)
                            strcat(reply_msg, " ERR\n");
                        else if (ERR == DIR_ERR)
                            strcat(reply_msg, " NOK\n");
                        else if (ERR == FILE_ERR){
                            strcat(reply_msg, " EOF\n");
                        }
                        else if (ERR == DUP_ERR)
                            strcat(reply_msg, " DUP\n");
                        else if (ERR == FULL_ERR)
                            strcat(reply_msg, " FULL\n");
                        n = write_buf_SIGPIPE(i, reply_msg, strlen(reply_msg));
                        TCP_FDS[user_atual].used = 1;
                        if (n == -1) {
                            //Conexao ja estava terminada
                            printf("User connection closed(sigpipe).\n");
                        }
                        else{
                            if (verbose_mode){
                                printf("Message sent to User\n");
                            }
                        }
                        retval--;
                        close(i);
                        FD_CLR(i, &active_fd_set);
                    }
                    else{
                   	 	// Se nao houve erro, manda mensagem ao AS para validar a transacao
                    	n_udp = sendto(udp_fd, msg, strlen(msg), 0, res_udp->ai_addr, res_udp->ai_addrlen);
                    	FD_SET(udp_fd, &active_fd_set);
                    	FD_CLR(i, &active_fd_set);
                    	AS_waiting_answer++;
                    	if (udp_fd > maxfd)
                        	maxfd = udp_fd;
                    	retval--;
                    }
                }
            }
        }
    }

    freeaddrinfo(res_udp);
    freeaddrinfo(res_tcp);
    close(udp_fd);
    close(tcp_fd);
}
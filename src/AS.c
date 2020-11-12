#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "msg.h"
#include "udpTimeout.h"

#define DEFAULT_PORT_AS "58011"
#define max(A, B) ((A) >= (B) ? (A) : (B))
#define MAX_NUM_U 100
#define DEFAULT_FILE_FOLDER "./Log/AsLog.txt"
#define TIMEOUT_DEFAULT 10

int fds, fd, tcp_fd, tcp_accept_fd, errcode;
ssize_t ns, n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct addrinfo hints_client_udp, *res_client_udp;
struct sockaddr_in addr;
struct sigaction act;
struct timeval tv;

char buffer[128];
char tcp_buffer[128];
char tcp_msg[128];
char udp_msg[128];
char udp_buffer[128];
int tcp_fd_array[MAX_NUM_U];
int numUsers = 0;
int tcpFdNumUsers = 0;
int TID = 0;

struct request_st {
    char RID[5];
    char VC[5];
    char op[1];
    char fileName[60];
    char TID[10];
    int vcUsed;
};

struct user_st {
    char uid[6];
    char pass[9];
    char pdIp[17];
    char pdPort[6];
    int isLogged;
    int numreq;
    int reqComplete;
    struct request_st arr_req[100];
};

/**
 * Funcao checkFileOp
 *  Descricao: 
 *      Funcao auxiliar para validação dos argumentos nas mensagens REQ.
 *      Verifica a consistencia entre tipo de operação e a existencia de argumento opcional
 * 
 *  Argumentos : 
 *      opOut -> tipo de operacao recebida nas mensagem
 *      argument -> argumento opcional recebino nas mensagens
 * 
 *  Retorno : 
 *      valor inteiro que representa verdadeiro(1) ou falso (0)
 * 
 **/

int checkFileOp(char *opOut, char *argument) {
    int result = 0;
    char fileOps[5] = {'L', 'R', 'U', 'D', 'X'};

    for (int i = 0; i < 5; i++)
        if (opOut[0] == fileOps[i]) {
            result = 1;
            break;
        }

    if ((opOut[0] == 'L' || opOut[0] == 'X') && strcmp(argument, "") != 0) {
        result = 0;
    } else if ((opOut[0] != 'L' && opOut[0] != 'X') && strcmp(argument, "") == 0) {
        result = 0;
    }

    return result;
}

/**
 * Funcao checkOpWithFile
 *  Descricao: 
 *      Funcao auxiliar para validação dos argumentos nas mensagens REQ.
 *      Verifica a se a operacao enviada por argumneto necessita do argumento opcional
 * 
 *  Argumentos : 
 *      opOut -> tipo de operacao recebida na mensagem
 * 
 * Retorno : 
 *      valor inteiro que representa verdadeiro(1) ou falso (0)
 * 
 **/

int checkOpWithFile(char opOut) {
    int result = 0;
    char fileOps[3] = {'R', 'U', 'D'};

    for (int i = 0; i < 3; i++)
        if (opOut == fileOps[i]) {
            result = 1;
            break;
        }

    return result;
}

/**
 * Funcao checkReqErr
 *  Descricao: 
 *      Funcao auxiliar que permite verificar se numero de elementos da mensagem
 *      esta de acordo com o protocolo
 * 
 *  Argumentos : 
 *      tcp_buffer -> string com a mensagem recebida pelo AS
 **/

int checkReqErr(char *tcp_buffer) {
    int result = 0;
    int num_tokens = 0;
    char testBuffer[30];
    char *token;

    strcpy(testBuffer, tcp_buffer);
    token = strtok(testBuffer, " ");

    while (token != NULL) {
        num_tokens++;
        token = strtok(NULL, "\n");
    }

    if (num_tokens != 5)
        result = 1;

    return result;
}

/**
 * Funcao structChecker
 *  Descricao: 
 *      Funcao util na monitorizacao do estado do servidor AS.
 *      Imprime o estado "base de dados" de utilizadores e dos respectivos 
 *      pedidos efetuados pelos mesmos
 * 
 *  Argumentos : 
 *      flagVerboseMode -> variavel recebida no inicio do programa que indica
 *                     se imprime a informação.
 * 
 *  Retorno : 
 *      valor inteiro que representa verdadeiro(1) ou falso (0)
 **/

void structChecker(int flagVerboseMode, struct user_st *arr_user) {
    if (!flagVerboseMode)
        return;
    printf("\n###### User DB ######\n");
    printf("\n Number of users: %d\n", numUsers);
    for (int j = 0; j < numUsers; j++) {
        printf(" User:%s pass:%s PD_ip:%s PD_port:%s isLogged?:%d numRequests:%d\n",
               arr_user[j].uid,
               arr_user[j].pass,
               arr_user[j].pdIp,
               arr_user[j].pdPort,
               arr_user[j].isLogged,
               arr_user[j].numreq);
        if (arr_user[j].numreq > 0) {
            for (int k = 0; k < arr_user[j].numreq; k++) {
                printf("   ->RID:%s VC:%s VCused?:%d OP:%c filename:%s TID:%s \n",
                       arr_user[j].arr_req[k].RID,
                       arr_user[j].arr_req[k].VC,
                       arr_user[j].arr_req[k].vcUsed,
                       arr_user[j].arr_req[k].op[0],
                       arr_user[j].arr_req[k].fileName,
                       arr_user[j].arr_req[k].TID);
            }
        }
    }
    printf("\n");
    printf("###### End User DB: ######\n\n");
}

/**
 * Funcao processSIGPIPE
 *  Descricao: 
 *      Funcao serve para terminar uma ligação tcp cancelada pelo user. 
 *      Depois de cancelada a ligação atualiza o array de file descriptors tcp.
 *      
 * 
 *  Argumentos : 
 *      tcpAcceptFd -> file descriptor do socket que foi terminado
 *      fdId -> posição do file descriptor no array de file descriptors tcp
 **/

void processSIGPIPE(int tcpAcceptFd, int fdId, int flagVerboseMode) {
    close(tcpAcceptFd);

    for (int i = 0; i < tcpFdNumUsers; i++) {
        if (fdId == i)
            tcp_fd_array[i] = tcp_fd_array[i + 1];
    }

    tcpFdNumUsers--;

    if (flagVerboseMode)
        printf("User connection closed.\n");
}

/**
 * Funcao verboseLogger
 *  Descricao: 
 *      Funcao auxiliar que prepara e imprime as mensagens recebidas via TCP e UDP.
 * 
 *  Argumentos : 
 *      verboseMode -> Variavel recebida no inicio do programa que indica se imprime a informação.
 *      buffer -> String com a mensagem recebida pelo AS
 *      loggerFlag -> Indica se a informação é IN ou OUT 
 *      fileFlag -> Indica se a informação do log vai ser colocada num ficheiro
 *      protocol -> Tipo de protocolo de comunicação UDP ou TCP
 **/

void verboseLogger(int flagVerboseMode, char *buffer, char *loggerFlag, char *fileFlag, char *protocol) {
    if (!flagVerboseMode)
        return;

    FILE *fp;
    char logMessage[500];

    if (strcmp(loggerFlag, "I") == 0) {
        sprintf(logMessage, "--> Received %s message: %s", protocol, buffer);

        printf("%s", logMessage);

        if (strcmp(fileFlag, "Y") == 0) {
            fp = fopen(DEFAULT_FILE_FOLDER, "a");
            fprintf(fp, "%s", logMessage);
            fclose(fp);
        }
    } else if (strcmp(loggerFlag, "O") == 0) {
        sprintf(logMessage, "<-- Sending %s message: %s", protocol, buffer);

        printf("%s", logMessage);

        if (strcmp(fileFlag, "Y") == 0) {
            fp = fopen(DEFAULT_FILE_FOLDER, "a");
            fprintf(fp, "%s", logMessage);
            fclose(fp);
        }
    }
}

int main(int argc, char *argv[]) {
    FILE *fp;
    int verboseMode = 0;
    fd_set rfds;
    int maxfd, maxfd1, retval;
    struct user_st arr_user[MAX_NUM_U];
    char hostname[1024];
    char logMessage[1000];

    char portAS[6] = DEFAULT_PORT_AS;
    srand(time(NULL));
    tv.tv_sec = TIMEOUT_DEFAULT;
    tv.tv_usec = 0;

    addrlen = sizeof(addr);

    if (argc == 1) {
        // default input
    } else if (argc == 2) {
        if (strcmp(argv[1], "-v") == 0) {
            verboseMode = 1;
        }
    } else if (argc == 3) {
        if (strcmp(argv[1], "-p") == 0) {
            stpcpy(portAS, argv[2]);
        }
    } else if (argc == 4) {
        if (strcmp(argv[1], "-p") == 0) {
            stpcpy(portAS, argv[2]);
        } else if (strcmp(argv[2], "-p") == 0) {
            stpcpy(portAS, argv[3]);
        }

        if (strcmp(argv[1], "-v") == 0) {
            verboseMode = 1;
        } else if (strcmp(argv[3], "-v") == 0) {
            verboseMode = 1;
        }
    } else {
        printf("Error in arguments!\n");
        return -1;
    }

    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    sprintf(logMessage, "AS server started at %s\nCurrently listening in port %s for UDP and TCP connections.\n\n", hostname, portAS);

    printf("%s%s", verboseMode == 1 ? "Verbose mode On!\n" : "Verbose mode Off!\n", logMessage);

    fp = fopen(DEFAULT_FILE_FOLDER, "w");
    fprintf(fp, "%s", logMessage);
    fclose(fp);

    // cria socket UDP servidor
    if ((fds = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        printf("Error: unable to create udp server socket\n");
        printf("Error code: %d\n", errno);
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, portAS, &hints, &res);
    if (errcode != 0)
        exit(1);

    ns = bind(fds, res->ai_addr, res->ai_addrlen);
    if (ns == -1) {
        printf("Error: unable to bind the udp server socket\n");
        printf("Error code: %d\n", errno);
        exit(1);
    }

    // cria socket UDP client
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        printf("Error: unable to create udp client socket\n");
        printf("Error code: %d\n", errno);
        exit(1);
    }

    memset(&hints_client_udp, 0, sizeof hints_client_udp);
    hints_client_udp.ai_family = AF_INET;
    hints_client_udp.ai_socktype = SOCK_DGRAM;

    // cria socket TCP servidor
    if ((tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Error: unable to create tcp server socket\n");
        printf("Error code: %d\n", errno);
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, portAS, &hints, &res);
    if (errcode == -1) {
        printf("Error: unable to bind the tcp server socket\n");
        printf("Error code: %d\n", errno);
        exit(1);
    }

    n = bind(tcp_fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        printf("Error: unable to bind the tcp server socket\n");
        printf("Error code: %d\n", errno);
        exit(1);
    }

    if (listen(tcp_fd, 5) == -1) {
        printf("Error: unable to set the server to listen\n");
        printf("Error code: %d\n", errno);
        exit(1);
    }

    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    signal(SIGPIPE, SIG_IGN);  // ignora os sigpipe vindos do cliente

    setTimeoutUDP(fd, TIMEOUT_DEFAULT);  // inicializa os timeouts para os sockets clientes

    while (1) {
        // inicializa os fds para comunicação UDP e registo de novos users em TCP
        FD_ZERO(&rfds);
        FD_SET(tcp_fd, &rfds);
        FD_SET(fd, &rfds);
        FD_SET(fds, &rfds);

        maxfd1 = max(tcp_fd, fds);
        maxfd = max(maxfd1, fd);

        // inicializa os fds para comunicação TCP com o cliente
        for (int fd_id = 0; fd_id <= tcpFdNumUsers; fd_id++) {
            FD_SET(tcp_fd_array[fd_id], &rfds);
            maxfd = max(maxfd, tcp_fd_array[fd_id]);
        }

        char msg[128] = "";
        char buffer[128] = "";

        retval = select(maxfd + 1, &rfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
        if (maxfd <= 0)
            exit(1);

        for (; retval; retval--) {
            if (FD_ISSET(tcp_fd, &rfds)) {
                // se receber novo registo cliente tcp, cria um novo fd a ser processado pelo select

                if ((tcp_accept_fd = accept(tcp_fd, (struct sockaddr *)&addr, &addrlen)) == -1) {
                    printf("%s %s\n", "error accept:", strerror(errno));
                    close(tcp_fd);
                    exit(1);
                }

                tcp_fd_array[tcpFdNumUsers] = tcp_accept_fd;
                tcpFdNumUsers++;

            } else if (FD_ISSET(fds, &rfds)) {
                // informação recebida por parte de um cliente UDP

                char op[5] = "";
                char user[6] = "";
                char pass[9] = "";
                char pdIP[17] = "";
                char pdPort[6] = "";

                ns = recvfrom(fds, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
                sscanf(buffer, "%s %s %s %s %s", op, user, pass, pdIP, pdPort);

                verboseLogger(verboseMode, buffer, "I", "Y", "UDP");

                // processa a mensagem e aplica as regras do protocolo UDP

                if (strcmp(op, "REG") == 0) {
                    if (strlen(user) == 5 && strlen(pass) == 8 && strcmp(pdIP, "") != 0 && strcmp(pdPort, "") != 0) {
                        struct user_st st_u;
                        struct request_st st_r;
                        int userUpdate = 0;

                        for (int i = 0; i < numUsers; i++) {
                            // Se for repetição de registo actualiza o par <ip,port> do PD
                            if (strcmp(arr_user[i].uid, user) == 0 && strcmp(arr_user[i].pass, pass) == 0 &&
                                strcmp(arr_user[i].pdIp, "") == 0) {
                                userUpdate = 1;
                                strcpy(arr_user[i].uid, user);
                                strcpy(arr_user[i].pass, pass);
                                strcpy(arr_user[i].pdIp, pdIP);
                                strcpy(arr_user[i].pdPort, pdPort);
                                arr_user[i].isLogged = 0;
                                arr_user[i].numreq = 0;
                                sprintf(msg, "RRG OK\n");

                                break;
                            } else if (strcmp(arr_user[i].uid, user) == 0 && strcmp(arr_user[i].pass, pass) != 0) {
                                // Se for repetição de registo e password estiver errada e retornada erro
                                userUpdate = -1;
                                sprintf(msg, "RRG NOK\n");
                            }
                        }

                        if (userUpdate == 0) {
                            // Se for um novo registo PD adiciona a estrutura
                            strcpy(st_u.uid, user);
                            strcpy(st_u.pass, pass);
                            strcpy(st_u.pdIp, pdIP);
                            strcpy(st_u.pdPort, pdPort);
                            st_u.isLogged = 0;
                            st_u.numreq = 0;
                            sprintf(msg, "RRG OK\n");
                            arr_user[numUsers] = st_u;
                            numUsers++;
                        }

                    } else {
                        // Se faltar algum campo na mesangem do protocolo
                        // ou os campos existentes nao estejam de acordo com o protocolo
                        sprintf(msg, "RRG NOK\n");
                    }
                } else if (strcmp(op, "UNR") == 0) {
                    struct user_st st_u;
                    int deleteUser = 0;
                    int deleteUserIndex = 0;

                    // Se o registo existir e se as credenciais
                    // estiverem corretas eliminar o registo da estrutura

                    for (int i = 0; i < MAX_NUM_U; i++) {
                        if (strcmp(arr_user[i].uid, user) == 0 && strcmp(arr_user[i].pass, pass) == 0) {
                            strcpy(arr_user[i].pdIp, "");
                            strcpy(arr_user[i].pdPort, "");
                            arr_user[i].isLogged = 0;
                            deleteUser = 1;
                        }
                    }

                    if (deleteUser) {
                        sprintf(msg, "RUN OK\n");
                    } else {
                        sprintf(msg, "RUN NOK\n");
                    }

                } else if (strcmp(op, "VLD") == 0) {
                    struct user_st st_u;
                    struct request_st st_r;
                    int tidVal = 0;
                    int uVal = 0;

                    for (int i = 0; i <= numUsers; i++) {
                        // Selecionar o utilizador para o pedido de validacao
                        if (strcmp(arr_user[i].uid, user) == 0) {
                            st_u = arr_user[i];
                            uVal = 1;
                            break;
                        }
                    }

                    for (int i = 0; i <= st_u.numreq; i++) {
                        // Se nao existir o user nao procurar o pedido
                        if (uVal == 0)
                            break;
                        // Seleciona o pedido a fazer validação
                        if (strcmp(st_u.arr_req[i].TID, pass) == 0) {
                            tidVal = 1;
                            st_r = st_u.arr_req[i];
                            break;
                        }
                    }

                    // Se não existir pedido ou utilizador devolve erro
                    if (tidVal == 0) {
                        sprintf(msg, "CNF %s %s E\n", user, pass);
                    } else if (checkOpWithFile(st_r.op[0])) {
                        // Se for um pedido com argumento opcional envia confirmacao
                        sprintf(msg, "CNF %s %s %c %s\n", user, st_r.TID, st_r.op[0], st_r.fileName);
                    } else {
                        // Se for um pedido sem argumento opcional
                        if (st_r.op[0] == 'X') {
                            // Se for um pedido X tem de eliminar todos os registos de pedidos do servidor AS
                            // envia a confirmação para o FS
                            char tempTID[10];
                            char tempOP;
                            int updateList = 0;

                            strcpy(tempTID, st_r.TID);
                            tempOP = st_r.op[0];

                            sprintf(msg, "CNF %s %s %c \n", user, st_r.TID, st_r.op[0]);

                            for (int i = 0; i <= numUsers; i++) {
                                if (strcmp(arr_user[i].uid, st_u.uid) == 0) {
                                    arr_user[i].numreq = 0;
                                    break;
                                }
                            }
                        } else {
                            // Caso contrario envia confirmacao de pedido sem argumento opcional
                            sprintf(msg, "CNF %s %s %c \n", user, st_r.TID, st_r.op[0]);
                        }
                    }
                } else {
                    // Mensagem nao prevista no protocolo UDP
                    sprintf(msg, "ERR\n");
                }

                verboseLogger(verboseMode, msg, "O", "Y", "UDP");
                structChecker(verboseMode, arr_user);

                ns = sendto(fds, msg, strlen(msg), 0, (struct sockaddr *)&addr, addrlen);

            } else {
                // Percorre todos os fd para TCP e seleciona um para processar
                for (int fd_id = 0; fd_id < tcpFdNumUsers; fd_id++) {
                    int tcp_accept_fd = tcp_fd_array[fd_id];

                    // Se o select detetar pedido ao fd selecionado
                    if (FD_ISSET(tcp_accept_fd, &rfds)) {
                        char op[5] = "";
                        char user[6] = "";
                        char arg1[9] = "";
                        char arg2[10] = "";
                        char arg3[100] = "";
                        char status[10] = "";
                        int uindex = 0;

                        n = read_buf(tcp_accept_fd, tcp_buffer, sizeof(tcp_buffer));

                        verboseLogger(verboseMode, tcp_buffer, "I", "Y", "TCP");

                        sscanf(tcp_buffer, "%s %s %s %s %s", op, user, arg1, arg2, arg3);

                        struct user_st st_u;
                        strcpy(st_u.uid, "");

                        // Se for um pedido de login verifica se o utilizador esta registado
                        // e se as credenciais estao corretas
                        if (strcmp(op, "LOG") == 0) {
                            for (int i = 0; i <= numUsers; i++) {
                                if (strcmp(arr_user[i].uid, user) == 0) {
                                    st_u = arr_user[i];
                                    uindex = i;
                                    break;
                                }
                            }

                            if (strcmp(st_u.uid, "") == 0) {  // Se user nao existir retorna erro
                                sprintf(tcp_msg, "RLO ERR\n");

                            } else if (strcmp(st_u.pass, arg1) != 0) {  // Se a password estiver errada retorna erro
                                sprintf(tcp_msg, "RLO NOK\n");

                            } else {  // Se o utilizador existir e a password estiver correta e assinalado como um user que fez login
                                arr_user[uindex].isLogged = 1;
                                sprintf(tcp_msg, "RLO OK\n");
                            }

                        } else if (strcmp(op, "REQ") == 0) {
                            struct user_st st_u;
                            strcpy(st_u.uid, "");

                            for (int i = 0; i <= numUsers; i++) {
                                if (strcmp(arr_user[i].uid, user) == 0) {
                                    st_u = arr_user[i];
                                    uindex = i;
                                    break;
                                }
                            }

                            if (checkReqErr(tcp_buffer) == 0) {
                                // Se o pedido estiver mal formatado devolve erro de protocolo
                                sprintf(tcp_msg, "ERR\n");
                            } else if (strcmp(st_u.uid, "") == 0) {
                                // Se o utilizador nao existir devolve erro
                                sprintf(tcp_msg, "RRQ EUSER\n");
                            } else {
                                char VC[5];
                                char *fileName = arg3;

                                // Cria um VC com 4 numeros aleatorios
                                sprintf(VC, "%d%d%d%d", rand() % 9, rand() % 9, rand() % 9, rand() % 9);

                                // Verifica se é um pedido com argumento opcional
                                if (checkOpWithFile(arg2[0]) == 0)
                                    sprintf(udp_msg, "VLC %s %s %s\n", user, VC, arg2);
                                else
                                    sprintf(udp_msg, "VLC %s %s %s %s\n", user, VC, arg2, arg3);

                                // prepara ligacao ao PD do user selecionado
                                errcode = getaddrinfo(st_u.pdIp, st_u.pdPort, &hints, &res);

                                verboseLogger(verboseMode, udp_msg, "O", "Y", "UDP");

                                n = sendto(fd, udp_msg, strlen(udp_msg), 0, res->ai_addr, res->ai_addrlen);

                                memset(udp_msg, 0, strlen(udp_msg));

                                n = recvfrom(fd, udp_msg, sizeof(udp_msg), 0, (struct sockaddr *)&addr, &addrlen);
                                if (checkTimeoutUdp(n) == -1)
                                    break;

                                sscanf(udp_msg, "%s %s", op, status);

                                verboseLogger(verboseMode, udp_msg, "I", "Y", "UDP");
                                structChecker(verboseMode, arr_user);

                                // Protocolo de resposta ao user
                                if (st_u.isLogged == 0) {
                                    // se user nao tiver feito login devolve erro para o user
                                    sprintf(tcp_msg, "RRQ ELOG\n");

                                } else if (strcmp(status, "NOK") == 0) {
                                    // se o PD devolver erro devolve o erro para o user
                                    sprintf(tcp_msg, "RRQ EPD\n");

                                } else if (checkFileOp(arg2, fileName) == 0) {
                                    // se o pedido estiver mal formatado consoante o tipo de operacao devolve erro para o user
                                    sprintf(tcp_msg, "RRQ EFOP\n");

                                } else {
                                    // caso contratio actualizar a estrutura dos pedidos para o user
                                    // e envia resposta ao user
                                    struct request_st st_r;
                                    char tidString[10];

                                    sprintf(tidString, "%d", ++TID);
                                    strcpy(st_r.RID, arg1);
                                    strcpy(st_r.op, arg2);
                                    strcpy(st_r.fileName, arg3);
                                    strcpy(st_r.VC, VC);
                                    strcpy(st_r.TID, tidString);
                                    st_r.vcUsed = 0;
                                    st_u.arr_req[st_u.numreq] = st_r;
                                    st_u.numreq++;

                                    arr_user[uindex] = st_u;
                                    sprintf(tcp_msg, "RRQ OK\n");
                                }
                            }
                        } else if (strcmp(op, "AUT") == 0) {
                            struct request_st st_r;
                            char tidString[500];
                            sprintf(tidString, "%d", 0);

                            // Verifica se o utilizador que pretende fazer a autorizacao existe
                            for (int i = 0; i <= numUsers; i++) {
                                if (strcmp(arr_user[i].uid, user) == 0) {
                                    st_u = arr_user[i];
                                    break;
                                }
                            }
                            // Verifica se tem o pedido que pretende autorizar
                            for (int i = 0; i <= st_u.numreq; i++) {
                                if (strcmp(st_u.arr_req[i].RID, arg1) == 0) {
                                    st_r = st_u.arr_req[i];
                                    break;
                                }
                            }
                            char vc[5];
                            strcpy(vc, st_r.VC);
                            // Verifica se o codigo de validacao esta correto
                            // e se nao foi utilizado previamente

                            if (strcmp(vc, arg2) == 0 && st_r.vcUsed == 0) {
                                strcpy(tidString, st_r.TID);
                                st_r.vcUsed = 1;

                                for (int i = 0; i <= st_u.numreq; i++) {
                                    if (strcmp(st_u.arr_req[i].RID, arg1) == 0) {
                                        st_u.arr_req[i] = st_r;
                                        break;
                                    }
                                }
                            }
                            // Envia o resultado da validacao do VC
                            // 0 em caso de VC invalido
                            // TID em caso de VC valido

                            sprintf(tcp_msg, "RAU %s\n", tidString);
                        } else {
                            sprintf(tcp_msg, "ERR\n");
                        }

                        // Envia mensagem para o User

                        n = write_buf_SIGPIPE(tcp_accept_fd, tcp_msg);

                        if (n == -1) {
                            // Em caso de erro processa o SIGPIPE
                            processSIGPIPE(tcp_accept_fd, fd_id, verboseMode);
                        } else {
                            verboseLogger(verboseMode, tcp_msg, "O", "Y", "TCP");
                            structChecker(verboseMode, arr_user);
                        }
                    }
                }
            }
            // limpa os buffers de memoria

            memset(tcp_buffer, 0, strlen(tcp_buffer));
            memset(buffer, 0, strlen(buffer));
            memset(tcp_msg, 0, strlen(tcp_msg));
            memset(udp_msg, 0, strlen(udp_msg));
            memset(udp_buffer, 0, strlen(udp_buffer));
            memset(msg, 0, strlen(msg));
        }
    }
    // Fecha os recursos utilizados pelas comunicacoes de sockets
    freeaddrinfo(res);
    freeaddrinfo(res_client_udp);
    close(fd);
    close(tcp_fd);

    return 0;
}

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

#define DEFAULT_PORT_AS "58011"
#define max(A, B) ((A) >= (B) ? (A) : (B))
#define MAX_NUM_U 30
#define DEFAULT_FILE_FOLDER "./Log/AsLog.txt"
int fds, fd, tcp_fd, tcp_accept_fd, errcode;
ssize_t ns, n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct addrinfo hints_client_udp, *res_client_udp;
struct sockaddr_in addr;
struct sigaction act;
char buffer[128];
char tcp_buffer[128];
char tcp_msg[128];
char udp_msg[128];
char udp_buffer[128];
int tcp_fd_array[MAX_NUM_U];
int numUsers = -1;
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
    struct request_st arr_req[100];
};

/**
 * Comment Template
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
 * Comment Template
 **/

int checkOpWithFile(char *opOut) {
    int result = 0;
    char fileOps[3] = {'R', 'U', 'D'};

    for (int i = 0; i < 3; i++)
        if (opOut[0] == fileOps[i]) {
            result = 1;
            break;
        }

    return result;
}

/**
 * Comment Template
 **/

void structChecker(struct user_st *arr_user) {
    printf("### Authentication Database: ###\n");
    printf("--> Number of users: %d\n", numUsers + 1);
    for (int j = 0; j <= numUsers; j++) {
        printf("--> user:%s pass:%s PD_ip:%s PD_port:%s isLogged?:%d numReques:%d\n",
               arr_user[j].uid,
               arr_user[j].pass,
               arr_user[j].pdIp,
               arr_user[j].pdPort,
               arr_user[j].isLogged,
               arr_user[j].numreq + 1);
        if (arr_user[j].numreq >= 0) {
            printf("\tNum of Requests: %d\n", arr_user[j].numreq + 1);
            for (int k = 0; k <= arr_user[j].numreq; k++) {
                printf("\t\t->RID:%s VC:%s OP:%c filename:%s TID:%s \n",
                       arr_user[j].arr_req[k].RID,
                       arr_user[j].arr_req[k].VC,
                       arr_user[j].arr_req[k].op[0],
                       arr_user[j].arr_req[k].fileName,
                       arr_user[j].arr_req[k].TID);
            }
        }
    }
}

/**
 * Comment Template
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
 * UDP Protocol function 
 * argumentos
 * TODO
 * 
 **/

int udpProtocol(char *tcp_buffer) {
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
 * Comment Template
 **/

void verboseLogger(int flagVerboseMode, char *buffer, char *loggerFlag, char *fileFlag, char *protocol) {
    if (!flagVerboseMode)
        return;

    FILE *fp;
    char logMessage[500];

    if (strcmp(loggerFlag, "I") == 0) {
        sprintf(logMessage, "--> Received %s message from : %s", protocol, buffer);

        printf("%s", logMessage);

        if (strcmp(fileFlag, "Y") == 0) {
            fp = fopen(DEFAULT_FILE_FOLDER, "a");
            fprintf(fp, "%s", logMessage);
            fclose(fp);
        }
    } else if (strcmp(loggerFlag, "O") == 0) {
        sprintf(logMessage, "<-- Sending %s message to : %s", protocol, buffer);

        printf("%s", logMessage);

        if (strcmp(fileFlag, "Y") == 0) {
            fp = fopen(DEFAULT_FILE_FOLDER, "a");
            fprintf(fp, "%s", logMessage);
            fclose(fp);
        }
    }
}

/**
 * Comment Template
 **/

void processSIGPIPE(int tcpAcceptFd, int fdId) {
    close(tcp_accept_fd);

    for (int i = fdId; i <= tcpFdNumUsers; i++) {
        if (fdId == tcpFdNumUsers)
            tcp_fd_array[i] = 0;
        else
            tcp_fd_array[i] = tcp_fd_array[i + 1];
    }

    tcpFdNumUsers--;
}

int main(int argc, char *argv[]) {
    char portAS[6] = DEFAULT_PORT_AS;
    int verboseMode = 0;
    fd_set rfds;
    int maxfd, maxfd1, retval;
    struct user_st arr_user[MAX_NUM_U];
    char hostname[1024];
    char logMessage[1000];
    srand(time(NULL));
    FILE *fp;

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
        printf("Error in argument setting!\n");
        return -1;
    }

    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    verboseMode == 1 ? "Verbose mode On!\n" : "Verbose mode Off!\n";

    sprintf(logMessage, "AS server started at %s\nCurrently listening in port %s for UDP and TCP connections.\n", hostname, portAS);

    printf("%s", logMessage);

    fp = fopen(DEFAULT_FILE_FOLDER, "w");
    fprintf(fp, "%s", logMessage);
    fclose(fp);

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

    // Create the udp client socket
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        printf("Error: unable to create udp client socket\n");
        printf("Error code: %d\n", errno);
        exit(1);
    }

    memset(&hints_client_udp, 0, sizeof hints_client_udp);
    hints_client_udp.ai_family = AF_INET;
    hints_client_udp.ai_socktype = SOCK_DGRAM;

    // Create the tcp server socket
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
    if (ns == -1) {
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
    signal(SIGPIPE, SIG_IGN);

    while (1) {
        // setting the select vars
        FD_ZERO(&rfds);
        FD_SET(tcp_fd, &rfds);
        FD_SET(fd, &rfds);
        FD_SET(fds, &rfds);
        FD_SET(0, &rfds);

        maxfd1 = max(tcp_fd, fds);
        maxfd = max(maxfd1, fd);

        for (int fd_id = 0; fd_id <= tcpFdNumUsers; fd_id++) {
            FD_SET(tcp_fd_array[fd_id], &rfds);
            maxfd = max(maxfd, tcp_fd_array[fd_id]);
        }

        // reset the message buffers in each iterarion
        char msg[128] = "";
        char buffer[128] = "";

        retval = select(maxfd + 1, &rfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
        if (maxfd <= 0)
            exit(1);

        for (; retval; retval--) {
            if (FD_ISSET(0, &rfds)) {
                char buffer[128];

                fgets(buffer, 128, stdin);

                if (strcmp(buffer, "AuthDB")) {
                    structChecker(arr_user);
                }

            } else if (FD_ISSET(tcp_fd, &rfds)) {
                if ((tcp_accept_fd = accept(tcp_fd, (struct sockaddr *)&addr, &addrlen)) == -1) {
                    printf("%s %s\n", "error accept:", strerror(errno));
                    close(tcp_fd);
                    exit(1);
                }

                tcpFdNumUsers++;
                tcp_fd_array[tcpFdNumUsers] = tcp_accept_fd;
            } else if (FD_ISSET(fds, &rfds)) {
                //server udp
                char op[5] = "";
                char user[6] = "";
                char pass[9] = "";
                char pdIP[17] = "";
                char pdPort[6] = "";

                ns = recvfrom(fds, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
                sscanf(buffer, "%s %s %s %s %s", op, user, pass, pdIP, pdPort);

                verboseLogger(verboseMode, buffer, "I", "Y", "UDP");

                if (strcmp(op, "REG") == 0) {
                    if (strlen(user) == 5 && strlen(pass) == 8 && strcmp(pdIP, "") != 0 && strcmp(pdPort, "") != 0) {
                        struct user_st st_u;
                        struct request_st st_r;
                        int userUpdate = 0;
                        int oldNumUsers = numUsers;

                        for (int i = 0; i < numUsers; i++) {
                            if (strcmp(arr_user[i].uid, user) == 0) {
                                numUsers = i;
                                userUpdate = 1;
                                break;
                            }
                        }

                        if (userUpdate == 0) {
                            numUsers++;
                        }

                        strcpy(st_u.uid, user);
                        strcpy(st_u.pass, pass);
                        strcpy(st_u.pdIp, pdIP);
                        strcpy(st_u.pdPort, pdPort);
                        st_u.isLogged = 0;
                        st_u.numreq = -1;

                        arr_user[numUsers] = st_u;

                        if (userUpdate != 0) {
                            numUsers = oldNumUsers;
                        }

                        sprintf(msg, "RRG OK\n");
                    } else {
                        sprintf(msg, "RRG NOK\n");
                    }
                } else if (strcmp(op, "UNR") == 0) {
                    struct user_st st_u;
                    int deleteUser = 0;
                    int deleteUserIndex = 0;

                    for (int i = 0; i < MAX_NUM_U; i++) {
                        if (strcmp(arr_user[i].uid, user) == 0 && strcmp(arr_user[i].pass, pass) == 0) {
                            deleteUser = 1;
                            deleteUserIndex = i;
                            numUsers--;
                            break;
                        }
                    }

                    if (deleteUser) {
                        for (int i = deleteUserIndex; i < numUsers; i++) {
                            arr_user[i] = arr_user[i + 1];
                        }

                        sprintf(msg, "RUN OK\n");
                    } else {
                        sprintf(msg, "RUN NOK\n");
                    }
                } else if (strcmp(op, "VLD") == 0) {
                    struct user_st st_u;
                    struct request_st st_r;
                    char op[2];
                    int tidVal = 0;
                    int uVal = 0;

                    for (int i = 0; i <= numUsers; i++) {
                        if (strcmp(arr_user[i].uid, user) == 0) {
                            st_u = arr_user[i];
                            uVal = 1;
                            break;
                        }
                    }

                    for (int i = 0; i <= st_u.numreq; i++) {
                        if (uVal == 0)
                            break;
                        if (strcmp(st_u.arr_req[i].TID, pass) == 0) {
                            tidVal = 1;
                            break;
                        }
                    }

                    if (tidVal == 0) {
                        sprintf(msg, "CNF %s %s E\n", user, pass);
                    } else if (checkOpWithFile(st_r.op)) {
                        sprintf(msg, "CNF %s %s %s\n", user, st_r.TID, st_r.op, st_r.fileName);
                    } else {
                        if (st_r.op[0] == 'X') {
                            struct request_st st_r;
                            char tempTID[10];
                            char tempOP[1];
                            int updateList = 0;

                            strcpy(tempTID, st_r.TID);
                            strcpy(tempOP, st_r.op);

                            for (int i = 0; i <= numUsers; i++) {
                                if (strcmp(arr_user[i].uid, st_u.uid) == 0) {
                                    updateList = 1;
                                }

                                if (updateList == 1) {
                                    arr_user[i - 1] = arr_user[i];
                                }
                            }
                            numUsers--;
                            sprintf(msg, "CNF %s %s\n", user, tempTID, tempOP);

                        } else {
                            sprintf(msg, "CNF %s %s %s\n", user, st_r.TID, st_r.op);
                        }
                    }
                } else {
                    sprintf(msg, "ERR\n");
                }
                verboseLogger(verboseMode, msg, "O", "Y", "UDP");
                ns = sendto(fds, msg, strlen(msg), 0, (struct sockaddr *)&addr, addrlen);
            } else {
                for (int fd_id = 0; fd_id <= tcpFdNumUsers; fd_id++) {
                    int tcp_accept_fd = tcp_fd_array[fd_id];

                    if (FD_ISSET(tcp_accept_fd, &rfds)) {
                        char op[5] = "";
                        char user[6] = "";
                        char arg1[9] = "";
                        char arg2[10] = "";
                        char arg3[10] = "";
                        char status[10] = "";
                        int uindex = 0;

                        n = read(tcp_accept_fd, tcp_buffer, sizeof(tcp_buffer));
                        verboseLogger(verboseMode, tcp_buffer, "I", "Y", "TCP");

                        sscanf(tcp_buffer, "%s %s %s %s %s", op, user, arg1, arg2, arg3);

                        struct user_st st_u;
                        strcpy(st_u.uid, "");
                        if (strcmp(op, "LOG") == 0) {
                            for (int i = 0; i <= numUsers; i++) {
                                if (strcmp(arr_user[i].uid, user) == 0) {
                                    st_u = arr_user[i];
                                    uindex = i;
                                    break;
                                }
                            }

                            if (strcmp(st_u.uid, "") == 0) {
                                sprintf(tcp_msg, "RLO ERR\n");
                            } else if (strcmp(st_u.pass, arg1) != 0) {
                                sprintf(tcp_msg, "RLO NOK\n");
                            } else {
                                arr_user[uindex].isLogged = 1;
                                sprintf(tcp_msg, "RLO OK\n");
                            }
                        } else if (strcmp(op, "REQ") == 0) {
                            struct user_st st_u;
                            for (int i = 0; i <= numUsers; i++) {
                                if (strcmp(arr_user[i].uid, user) == 0) {
                                    st_u = arr_user[i];
                                    uindex = i;
                                    break;
                                }
                            }

                            if (checkReqErr(tcp_buffer) == 0) {
                                sprintf(tcp_msg, "ERR\n");
                            } else if (strcmp(st_u.uid, "") == 0) {
                                sprintf(tcp_msg, "RRQ EUSER\n");
                            } else {
                                char VC[5];
                                sprintf(VC, "%d%d%d%d", rand() % 9, rand() % 9, rand() % 9, rand() % 9);

                                sprintf(udp_msg, "VLC %s %s %s %s\n", user, VC, arg2, arg3);

                                char *fileName = arg3;
                                errcode = getaddrinfo(st_u.pdIp, st_u.pdPort, &hints, &res);

                                verboseLogger(verboseMode, udp_msg, "O", "Y", "UDP");
                                n = sendto(fd, udp_msg, strlen(udp_msg), 0, res->ai_addr, res->ai_addrlen);

                                memset(udp_msg, 0, strlen(udp_msg));

                                n = recvfrom(fd, udp_msg, sizeof(udp_msg), 0, (struct sockaddr *)&addr, &addrlen);
                                sscanf(udp_msg, "%s %s", op, status);
                                verboseLogger(verboseMode, udp_msg, "I", "Y", "UDP");

                                if (st_u.isLogged == 0) {
                                    sprintf(tcp_msg, "RRQ ELOG\n");

                                } else if (strcmp(status, "NOK") == 0) {
                                    sprintf(tcp_msg, "RRQ EPD\n");

                                } else if (checkFileOp(arg2, fileName) == 0) {
                                    sprintf(tcp_msg, "RRQ EFOP\n");

                                } else {
                                    struct request_st st_r;
                                    char tidString[10];
                                    st_u.numreq++;

                                    sprintf(tidString, "%d", ++TID);
                                    strcpy(st_r.RID, arg1);
                                    strcpy(st_r.op, arg2);
                                    strcpy(st_r.fileName, arg3);
                                    strcpy(st_r.VC, VC);
                                    strcpy(st_r.TID, tidString);
                                    st_r.vcUsed = 0;

                                    st_u.arr_req[st_u.numreq] = st_r;
                                    arr_user[uindex] = st_u;

                                    sprintf(tcp_msg, "RRQ OK\n");
                                }
                            }
                        } else if (strcmp(op, "AUT") == 0) {
                            struct request_st st_r;
                            char tidString[500];

                            for (int i = 0; i <= numUsers; i++) {
                                if (strcmp(arr_user[i].uid, user) == 0) {
                                    st_u = arr_user[i];
                                    break;
                                }
                            }

                            for (int i = 0; i <= st_u.numreq; i++) {
                                if (strcmp(st_u.arr_req[i].RID, arg1) == 0) {
                                    st_r = st_u.arr_req[i];
                                    break;
                                }
                            }

                            if (strcmp(st_r.VC, arg2) == 0 && st_r.vcUsed == 0) {
                                strcpy(tidString, st_r.TID);
                                st_r.vcUsed = 1;
                            } else {
                                sprintf(tidString, "%d\0", 0);
                            }
                            sprintf(tcp_msg, "RAU %s\n", tidString);
                        } else {
                            sprintf(tcp_msg, "ERR\n");
                        }

                        n = write(tcp_accept_fd, tcp_msg, strlen(tcp_msg));

                        if (n == -1)
                            processSIGPIPE(tcp_accept_fd, fd_id);
                        else
                            verboseLogger(verboseMode, tcp_msg, "O", "Y", "TCP");
                    }
                }
            }

            memset(tcp_buffer, 0, strlen(tcp_buffer));
            memset(buffer, 0, strlen(buffer));
            memset(tcp_msg, 0, strlen(tcp_msg));
            memset(udp_msg, 0, strlen(udp_msg));
            memset(udp_buffer, 0, strlen(udp_buffer));
            memset(msg, 0, strlen(msg));
        }
    }
    return 0;
}

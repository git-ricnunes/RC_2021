# makefile
CC = gcc
CDFLAGS =-g -o
LDFLAGS =
OBJS = pd User AS FS Log
MKDIR_LOG = mkdir Log

all:
	rm -rf Log/
	$(MKDIR_LOG)
	$(CC) $(CDFLAGS) pd src/PD.c src/udpTimeout.c
	$(CC) $(CDFLAGS) User src/User.c src/msg.c
	$(CC) $(CDFLAGS) AS src/AS.c src/msg.c src/udpTimeout.c
	$(CC) $(CDFLAGS) FS src/FS.c src/msg.c
pd: 
	$(CC) $(CDFLAGS) pd src/PD.c
User:
	$(CC) $(CDFLAGS) User src/User.c src/msg.c
AS:
	rm -rf Log/
	$(MKDIR_LOG)
	$(CC) $(CDFLAGS) AS src/AS.c src/msg.c src/udpTimeout.c
FS: 
	$(CC) $(CDFLAGS) FS src/FS.c
clean:
	rm -rf $(OBJS)
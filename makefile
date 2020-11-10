# makefile
CC = gcc
CFLAGS = -Wall -g
LDFLAGS =
OBJS = pd User AS FS Log
MKDIR_LOG = mkdir Log

all:
	rm -rf Log/
	$(MKDIR_LOG)	
	$(CC) $(CDFLAGS) -o pd src/PD.c 
	$(CC) $(CDFLAGS) -o User src/User.c
	$(CC) $(CDFLAGS) -o AS src/AS.c
	$(CC) $(CDFLAGS) -o FS src/FS.c
pd: 
	$(CC) $(CDFLAGS) -o pd src/PD.c
User:
	$(CC) $(CDFLAGS) -o User src/User.c
AS:
	rm -rf Log/
	$(MKDIR_LOG)
	$(CC) $(CDFLAGS) -o AS src/AS.c
FS: 
	$(CC) $(CDFLAGS) -o FS src/FS.c
clean:
	rm -rf $(OBJS)
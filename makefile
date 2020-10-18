# makefile
CC = gcc
CFLAGS = -Wall -g
LDFLAGS =
OBJS = pd User AS FS
all:
	$(CC) $(CDFLAGS) -o pd src/PD.c 
	$(CC) $(CDFLAGS) -o User src/User.c
	$(CC) $(CDFLAGS) -o AS src/AS.c
	$(CC) $(CDFLAGS) -o FS src/FS.c
pd: 
	$(CC) $(CDFLAGS) -o pd src/PD.c
User:
	$(CC) $(CDFLAGS) -o User src/User.c
AS: 
	$(CC) $(CDFLAGS) -o AS src/AS.c
FS:
	$(CC) $(CDFLAGS) -o FS src/FS.c
clean:
	rm -rf $(OBJS)
# makefile
CC = gcc
CFLAGS = -Wall -g
LDFLAGS =

all:
	$(CC) $(CDFLAGS) -o pd PD/PD.c 
	$(CC) $(CDFLAGS) -o User User/User.c
pd: 
	$(CC) $(CDFLAGS) -o pd PD/PD.c
User:
	$(CC) $(CDFLAGS) -o User User/User.c

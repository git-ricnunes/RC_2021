# makefile

all:
	gcc -Wall -g -o pd PD/PD.c 
	gcc -Wall -g -o User User/User.c -lm
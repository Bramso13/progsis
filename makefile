CC = gcc
OPTIONS = -Wall

all : initial serveurs cuisiniers clients

initial : initial.c
	$(CC) $(OPTIONS) initial.c -o initial

serveurs : serveurs.c
	$(CC) $(OPTIONS) serveurs.c -o serveurs

cuisiniers : cuisiniers.c 
	$(CC) $(OPTIONS) cuisiniers.c -o cuisiniers
	
clients : clients.c
	$(CC) $(OPTIONS) clients.c -o clients


clean :
	rm -f initial serveurs cuisiniers clients


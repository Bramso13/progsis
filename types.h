
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define FICHIER_CLE "cle.serv"

#define LETTRE_CODE 'a'


struct message{
    long type; /* 1 pour serveur, 2 pour cuisiniers et 3 pour clients */
    int attente; /* 0 ou 1 */
    int commande;
    int monid;
    int client;
};


struct data{
    int nb_spec;
    int nb_categorie;
    int tab[100][100];
    int ustensile[100];
    int tpe[100];
    int nb_tpe;
};

/* Couleurs dans xterm */
#define couleur(param) fprintf(stdout,"\033[%sm",param)

#define NOIR  "30"
#define ROUGE "31"
#define VERT  "32"
#define JAUNE "33"
#define BLEU  "34"
#define CYAN  "36"
#define BLANC "37"
#define REINIT "0"
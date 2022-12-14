#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>

#include "types.h"

void usage(){
    printf("probleme param client\n");
    exit(-1);
}
void arret(int s){

    exit(0);
}

void mon_sigaction(int signal, void (*f)(int)){
    struct sigaction action;
 
    action.sa_handler = f;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(signal,&action,NULL);
}

int main(int argc, char ** argv, char ** envp){
    key_t cle; /* cle de la file     */
    int file_mess; /* ID de la file    */
    FILE *fich_cle;
    int pid;
    int res_rcv;
    struct message mg;
    struct message rep;
    
    pid = getpid();

    srand(time(NULL));
    if(argc < 2) usage();
    int nb_spec = atoi(argv[1]);
    int pif = rand()%(nb_spec-1) +1;
    printf("Choix de la commande : %d\n", pif);

    fich_cle = fopen(FICHIER_CLE,"r");
    if (fich_cle==NULL){
	fprintf(stderr,"Lancement client impossible\n");
	exit(-1);
    }
    
    cle = ftok(FICHIER_CLE,'a');
    if (cle==-1){
	fprintf(stderr,"Pb creation cle\n");
	exit(-1);
    }

    file_mess = msgget(cle,0);
    if (file_mess==-1){
        fprintf(stderr,"Pb recuperation file de message\n");
        exit(-1);
    }
    mon_sigaction(SIGINT, arret);
    mg.attente = 0;
    mg.type = 1;
    mg.commande = pif;
    mg.client = pid;
    mg.client = pid;
    int e = msgsnd(file_mess, &mg, sizeof(struct message), 0);
    if(e == -1){
        fprintf(stderr,"Erreur, numero %d\n",errno);
        exit(-1);
    }

    /* attente de la reponse :                        */
    printf("(client %d) Attente de l'attribution du serveur le moins occupé...\n", pid);
    res_rcv = msgrcv(file_mess,&rep,sizeof(struct message),pid,0);
    if (res_rcv ==-1){
        fprintf(stderr,"Erreur, numero %d\n",errno);
        exit(-1);
    }
    /* attente de l'attribution du serveur le moins occupé */

    printf("(client %d) Attribution d'un serveur effectuée.\n", pid);
    res_rcv = msgrcv(file_mess,&rep,sizeof(struct message),pid,0);
    if (res_rcv ==-1){
        fprintf(stderr,"Erreur, numero %d\n",errno);
        exit(-1);
    }
    sleep(1);


    /* attente de la fabrication de la commande */
    printf("(client %d) En attente de la commande...\n", pid);
    res_rcv = msgrcv(file_mess,&rep,sizeof(struct message),pid,0);
    if (res_rcv ==-1){
        fprintf(stderr,"Erreur, numero %d\n",errno);
        exit(-1);
    }
    couleur(BLEU);
    printf("(client %d) Commande reçu.\n", pid);
    couleur(REINIT);
    printf("(client %d) Paiement en cours.\n", pid);
    mg.attente = 0;
    mg.type = rep.monid;
    mg.commande = pif;
    mg.client = pid;
    mg.monid = pid;
    e = msgsnd(file_mess, &mg, sizeof(struct message), 0);
    if(e == -1){
        fprintf(stderr,"Erreur, numero %d\n",errno);
        exit(-1);
    }

    
    
    /* Fin normale                                */
    exit(0);
    

}
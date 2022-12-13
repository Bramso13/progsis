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

int main(int argc, char ** argv, char ** envp){
    key_t cle; /* cle de la file     */
    int file_mess; /* ID de la file    */
    FILE *fich_cle;
    int pid;
    int res_rcv;
    struct message mg;
    struct message rep;
    
    pid = getpid();


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
    printf("(client) En attente de la réponse...\n");
    res_rcv = msgrcv(file_mess,&rep,sizeof(struct message),pid,0);
    if (res_rcv ==-1){
        fprintf(stderr,"Erreur, numero %d\n",errno);
        exit(-1);
    }
    /* attente de l'attribution du serveur le moins occupé */
    printf("Réponse : %d %d %ld %d %d\n", rep.attente, rep.commande, rep.type, rep.client, pid);
    printf("(%d) Attente de l'attribution du serveur le moins occupé...\n", pid);
    res_rcv = msgrcv(file_mess,&rep,sizeof(struct message),pid,0);
    if (res_rcv ==-1){
        fprintf(stderr,"Erreur, numero %d\n",errno);
        exit(-1);
    }
    sleep(1);
    printf("(%d) Attribution d'un serveur effectuée.\n", pid);
    printf("Réponse : %d %d %ld %d %d\n", rep.attente, rep.commande, rep.type, rep.client, pid);
    /* attente de la fabrication de la commande */
    printf("(%d) En attente de la commande...\n", pid);
    res_rcv = msgrcv(file_mess,&rep,sizeof(struct message),pid,0);
    if (res_rcv ==-1){
        fprintf(stderr,"Erreur, numero %d\n",errno);
        exit(-1);
    }
    couleur(BLEU);
    printf("(%d) Commande reçu.\n", pid);
    couleur(REINIT);
    printf("Réponse : %d %d %ld %d %d\n", rep.attente, rep.commande, rep.type, rep.client, pid);
    
    
    /* Fin normale                                */
    exit(0);
    

}
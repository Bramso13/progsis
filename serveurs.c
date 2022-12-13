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
    int pid, i;
    int res_rcv;
    int mem_part; 
    int semap;  
    struct data * tab;
    struct message mg;
    struct message rep;
    srand(time(NULL));
    pid = getpid();
    struct sembuf P = {0,-1,SEM_UNDO}; /* Operation P        */
    struct sembuf V = {0,1,SEM_UNDO};  /* Operation V        */

    if(argc < 2) usage();
    int nb_ordre = atoi(argv[1]);
    

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

    /* Recuperation SMP :    */
    mem_part = shmget(cle,sizeof(struct data),0);
    if (mem_part == -1){
	printf("(Fils %d) Pb recuperation SMP\n",pid);
	exit(-1);
    }

    /* Attachement SMP :      */
    tab = shmat(mem_part,NULL,0);
    if (tab == (struct data *)-1){
	printf("(Fils %d) Pb attachement SMP\n",pid);
	exit(-1);
    }
    /* Recuperation semaphore :         */
    semap = semget(cle,1,0);
    if (semap == -1){
	printf("(Fils %d) Pb recuperation semaphore\n",pid);
	exit(-1);
    }

    int p;
    while(1){
        p=1;
        /* attente de la reponse :                        */
        printf("(serveur %d) En attente d'une commande...\n", nb_ordre);
        res_rcv = msgrcv(file_mess,&rep,sizeof(struct message),1,0);
        if (res_rcv ==-1){
            fprintf(stderr,"Erreur, numero %d\n",errno);
            exit(-1);
        }
        mg.attente = 2;
        mg.type = rep.client;
        mg.commande = rep.commande;
        mg.monid = pid;
        mg.client = rep.client;
        int e = msgsnd(file_mess, &mg, sizeof(struct message), 0);
        if(e == -1){
            fprintf(stderr,"Erreur, numero %d\n",errno);
            exit(-1);
        }
        
        if(rep.attente == 1 ){/* Commande réaliser par l'un des cuisiniers */
            /* Reservation d'un tpe */
            
           
            printf("Réservation d'un tpe...\n");
            /* Debut section critique                 */
            /* envoie d'un message au client pour le paiement */
            mg.attente = 2;
            mg.type = rep.client;
            mg.commande = rep.commande;
            mg.monid = pid;
            mg.client = rep.client;
            int e = msgsnd(file_mess, &mg, sizeof(struct message), 0);
            if(e == -1){
                fprintf(stderr,"Erreur, numero %d\n",errno);
                exit(-1);
            }
            //tab->tpe[i] = 0;
            sleep(1);
           
            printf("Commande prête !\n");
        }else if(rep.attente == 0 ){
            /* envoie d'un message au cuisiniers */
            printf("Envoi ordre à un cuisinier...\n");
            mg.attente = 0;
            mg.type = 2;
            mg.commande = rep.commande;
            mg.monid = pid;
            mg.client = rep.client;
            int e = msgsnd(file_mess, &mg, sizeof(struct message), 0);
            if(e == -1){
                fprintf(stderr,"Erreur, numero %d\n",errno);
                exit(-1);
            }
            sleep(1);
        }
    }


    exit(0);

}
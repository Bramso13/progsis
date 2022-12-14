#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>
#include <semaphore.h>

#include "types.h"

int nb_ordre;
int nbCommandeF;

void usage(){
    printf("probleme param client\n");
    exit(-1);
}
void arret(int s){
    
    couleur(BLEU);
    printf("(Cuisinier %d) Nombre de commande : %d\n", nb_ordre, nbCommandeF);
    couleur(REINIT);
    
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
    int mem_part;   /* ID du SMP                             */
    int semap;      /* ID de l'ensemble de semaphores        */
    struct data * tab; /* SMP */
    int pid = getpid(); /* pid du cuisinier */
    int res_rcv;
    struct message mg;
    struct message rep;
    struct sembuf P = {0,-1,IPC_NOWAIT}; /* Operation P        */
    struct sembuf V = {0,1,IPC_NOWAIT};  /* Operation V        */

    int nb_spec = atoi(argv[2]);
    int nb_cat = atoi(argv[3]);
    nb_ordre = atoi(argv[1]);

    
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
    mon_sigaction(SIGINT, arret);
    int j;
    while(1){
        /* attente de la reponse :                        */
        printf("(cuisine) En attente de la réponse...\n");
        res_rcv = msgrcv(file_mess,&rep,sizeof(struct message),2,0);
        if (res_rcv ==-1){
            fprintf(stderr,"Erreur, numero %d\n",errno);
            exit(-1);
        }
        for(j=0;j<nb_cat;j++){
            while(tab->tab[rep.commande][j] > tab->ustensile[j]){
                //printf("attente d'un ustensile\n");
            }
        }

        /* Attente semaphore                      */
        semop(semap,&P,1);
        /* Debut section critique                 */

        printf("Reservation des ustensiles...\n");
        for(j=0;j<nb_cat;j++){
            tab->ustensile[j] = tab->ustensile[j] - tab->tab[rep.commande][j];
        }
        sleep(rand()%10); // Fabrication de la commande 
        printf("Fin Reservation des ustensiles...\n");
        for(j=0;j<nb_cat;j++){
            tab->ustensile[j] = tab->ustensile[j] + tab->tab[rep.commande][j];
        }
        /* Fin section critique                   */
        /* Liberation semaphore                   */
        semop(semap,&V,1);
        
        printf("(%d) Commande prête ! (cuisinier %d)\n", rep.client, nb_ordre);
        mg.attente = 1;
        mg.type = 1;
        mg.commande = rep.commande;
        mg.monid = rep.monid;
        mg.client = rep.client;
        int e = msgsnd(file_mess, &mg, sizeof(struct message), 0);
        if(e == -1){
            fprintf(stderr,"Erreur, numero %d\n",errno);
            exit(-1);
        }
        nbCommandeF++;
        /* Pour etre (un peu)aléatoire            */
	    sleep(rand()%3);
    }


    exit(0);

}
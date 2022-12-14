
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>

#include "types.h"

#define MAX_FILS 100
#define MAX_USTENSILE 100

int file_mess; /* ID de la file, necessairement global
		  pour pouvoir la supprimer a la terminaison */
int mem_part;         /* ID du SMP                           */
int semap;            /* ID de l'ES                          */
struct data * tab;

void usage(char s[]){
    printf("Usage : %s (2 < nb_serveurs < %d) (2 < nb_cuisiniers < %d) nb_terms nb_spec (nb_1 ... nb_k)\n",s, MAX_FILS, MAX_FILS);
    exit(-1);
}
void arret(int s){
    couleur(ROUGE);
    fprintf(stdout,"Serveur s'arrete (SIGINT recu)\n");
    couleur(BLEU);
    /* Affichage du chiffre d'affaire réalisé */
    printf("Chiffre d'affaire : %ld€\n", tab->chiffreAffaire);
    /*Affichage des prix de chaque spécialités */
    int p;
    for(p=0;p<tab->nb_spec;p++){
        printf("Prix spécialité %d : %d€\n", p+1, tab->tab[p][tab->nb_categorie+1]);
    }
    couleur(REINIT);

    shmdt(tab);

    /* Destruction SMP */
    shmctl(mem_part,IPC_RMID,NULL);

    /* Destruction ensemble de semaphores         */
    semctl(semap,1,IPC_RMID,NULL);
    /* Destruction file de message */
    msgctl(file_mess, IPC_RMID, NULL);
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
    struct stat st;       /* pour le stat sur le fichier cle     */
    key_t cle;            /* cle des IPC                         */
                 /* @ attachement SMP                   */
    int encore;
    pid_t pid;            /* pour l'affichage                    */
    char chi[1024];       /* pour fabriquer les argv des fils    */
    unsigned short val_init[1]={1};
                          /* valeur initiale du semaphore        */

    int tabPid[30];
    memset(tabPid, 0, sizeof(int)*30);
    srand(time(NULL));
    if(argc < 6)
        usage(argv[0]);
    int nb_serveurs, nb_cuisiniers, nb_terms, nb_spec, i;
    int ustensile[MAX_USTENSILE];
    memset(ustensile, 0, sizeof(int)*MAX_USTENSILE);

    nb_cuisiniers = atoi(argv[2]);
    nb_serveurs = atoi(argv[1]);
    nb_terms = atoi(argv[3]);
    nb_spec = atoi(argv[4]);


    if(nb_terms >= nb_serveurs) usage("nb_term >= nb_serveur");
    if(nb_cuisiniers <= 0 || nb_serveurs <= 0 || nb_terms <= 0 || nb_spec <= 0) 
        usage("<= 0");
    int tabPidServ[30];
    memset(tabPidServ, 0, sizeof(int)*nb_serveurs);
    int tabPidCuis[30];
    memset(tabPidCuis, 0, sizeof(int)*nb_cuisiniers);
    
    for(i=0;i<argc-5;i++){
        ustensile[i] = atoi(argv[5+i]);
    }

    /* Creation de la cle :          */
    /* 1 - On teste si le fichier cle existe dans le repertoire courant : */
    if ((stat(FICHIER_CLE,&st) == -1) &&
	(open(FICHIER_CLE, O_RDONLY | O_CREAT | O_EXCL, 0660) == -1)){
        fprintf(stderr,"Pas de fichier cle et pb creation fichier cle, bye\n");
        exit(-1);
    }

    cle = ftok(FICHIER_CLE,LETTRE_CODE);
    if (cle == -1){
        printf("Pb creation cle\n");
        exit(-1);
    }

    /* On cree le SMP et on teste s'il existe deja :    */
    mem_part = shmget(cle,sizeof(struct data),IPC_CREAT | IPC_EXCL | 0660);
    if (mem_part == -1){
        printf("Pb creation SMP ou il existe deja\n");
        exit(-1);
    }
    
    /* Attachement de la memoire partagee :          */
    tab = shmat(mem_part,NULL,0);
    if (tab == (struct data *) -1){
        printf("Pb attachement\n");
        /* Il faut detruire le SMP puisqu'on l'a cree : */
        shmctl(mem_part,IPC_RMID,NULL);
        exit(-1);
    }

    /* On cree le semaphore (meme cle) :                     */
    semap = semget(cle,1,IPC_CREAT | IPC_EXCL | 0660);
    if (semap == -1){
        printf("Pb creation ensemble de semaphore ou il existe deja\n");
        /* Il faut detruire le SMP puisqu'on l'a cree : */
        shmctl(mem_part,IPC_RMID,NULL);
        /* Le detachement du SMP se fera a la terminaison */
        exit(-1);
    }

    /* On l'initialise :                                     */
    if (semctl(semap,1,SETALL,val_init) == -1){
        printf("Pb initialisation semaphore\n");
        /* On detruit les IPC deje crees : */
        semctl(semap,1,IPC_RMID,NULL);
        shmctl(mem_part,IPC_RMID,NULL);
        exit(-1);
    }
    /* Creation file de message :    */
    file_mess = msgget(cle,IPC_CREAT | IPC_EXCL | 0660);
    if (file_mess==-1){
        fprintf(stderr,"Pb creation file de message\n");
        exit(-1);
    }

    /* Tout est OK, on initialise :                          */
    tab->nb_categorie = argc - 5;
    tab->nb_spec = nb_spec;
    tab->chiffreAffaire = 0;
    memset(tab->tpe, 0, sizeof(int)*nb_terms);
    tab->nb_tpe = nb_terms;
    int j, k;
    for(j=0;j<nb_spec;j++){
        for(k=0;k<argc-5;k++){
            tab->tab[j][k] = rand()%(ustensile[k]);
            tab->ustensile[k] = ustensile[k];

        }
        tab->tab[j][tab->nb_categorie+1] = rand()%(15) + 5; /* Prix */

    }
    mon_sigaction(SIGINT,arret);
    /* Lancement des serveurs */
    printf("Lancement des serveurs\n");
    for(i=0;i<nb_serveurs;i++){
        fprintf(stderr,".");
        sprintf(chi,"%d",i+1);
        pid = fork();
        
        if (pid == -1)  /* Probleme a la creation du
                i-ieme fils, on arrete les fork */
            break;
        if (pid == 0){
            /* c'est le i-eme fils   */
            execl("serveurs", "serveurs", chi, NULL);
            /* en principe jamais atteint */
            exit(-1);
        }
        tabPidServ[i] = pid;
    }
    fprintf(stderr,"fait\n");
    sleep(3);
    char argD[1024]; /* argv[1] de cuisiniers*/
    char argT[1024]; /* argv[2] de cuisiniers*/
    printf("Lancement des cuisiniers\n");
    for(i=0;i<nb_cuisiniers;i++){
        fprintf(stderr,".");
        sprintf(chi,"%d",i+1);
        sprintf(argD, "%d", nb_spec);
        sprintf(argT, "%d", argc-5);
        pid = fork();

        if (pid == -1)  /* Probleme a la creation du
                i-ieme fils, on arrete les fork */
            break;
        if (pid == 0){
            /* c'est le i-eme fils   */
            execl("cuisiniers", "cuisiniers", chi, argD, argT, NULL);
            /* en principe jamais atteint */
            exit(-1);
        }
        tabPidCuis[i] = pid;
    }
    fprintf(stderr,"fait\n");
    sleep(3);
    /* Boucle infini des clients */
    while(1){
        /* On lance les clients :                                */
        fprintf(stderr,"Lancement des clients");
        int pifo = rand()%20 + 1;
        memset(tabPid, 0, sizeof(int)*21);
        for(i=0;i<pifo;i++){
            fprintf(stderr,".");
            sprintf(chi,"%d",nb_spec);
            pid = fork();
            
            if (pid == -1)  /* Probleme a la creation du
                    i-ieme fils, on arrete les fork */
                break;
            if (pid == 0){
                /* c'est le i-eme fils   */
                execl("clients", "clients", chi, NULL);
                /* en principe jamais atteint */
                exit(-1);
            }
            tabPid[i] = pid;
            sleep(3);
        }
        fprintf(stderr,"fait\n");
        sleep(5);

        int t=0;
        
        /* On les attend :                            */
        
        fprintf(stderr,"Attente (robuste) de leur terminaison %d", pifo);
        encore = 1;
        while (encore)
            if ((waitpid(-1,NULL,0) == -1) && (errno == ECHILD))
                encore = 0;
            else{
                t++;
                fprintf(stderr,".");
                if(t == pifo) { /* On attends que chaque client soit bien fini */
                    encore = 0;
                    t=0;
                }
            }

        fprintf(stderr," c'est fini.\n");

       
        
        sleep(1);
    }
    /* On nettoie :                               */
    /* Detachement SMP */
    shmdt(tab);

    /* Destruction SMP */
    shmctl(mem_part,IPC_RMID,NULL);

    /* Destruction ensemble de semaphores         */
    semctl(semap,1,IPC_RMID,NULL);
    /* Destruction file de message */
    msgctl(file_mess, IPC_RMID, NULL);

    return 0;

}

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


#define INIT 100
#define LOOKUP 101
#define LASTCHANCE 102
#define ANSWER 103
#define CONNECT 104
#define DISCONNECT 105
#define DISCONNECT1 106

#define NB_SITE 5
#define K 8

/*
 Compilation :  mpicc -o chord chord.c
 Execution	:  mpirun -np 6 chord
 
 Le nombre de site est modifiable
 
 Dans ce programme, le proc0 va initialiser les variables locales de chaque proc.
 Ensuite, le proc1 va chercher une donnée, puis quand il recoit cette donnée, il va se deconnecter.
 Ensuite, le predecesseur de proc1 va réaliser une nouvelle recherche.
 
 Ainsi de suite jusqu'à ce qu'il ne reste que un processus.
 
 Les clés CHORD sont choisis aléatoirement par le proc0.
 
 Ce programme utilise beaucoup la fonction sleep() afin de ralentir son execution et d'avoir une trace lisible.
 Ce programme gere la deconnexion, mais pas l'ajout de nouveaux pairs.
 */
int cles[NB_SITE+1] = {-1}; //A MODIF EN TIRAGE ALEATOIRE // Ces variables globales ne sont accedées que par le proc0
int premiereDonnee[NB_SITE+1] = {-1};
int successeur[NB_SITE+1] = {-1};

/*	int cles[NB_SITE+1] = {-1, 3, 6, 9, 12, 15}; //A MODIF EN TIRAGE ALEATOIRE // Ces variables globales ne sont accedées que par le proc0
 int premiereDonnee[NB_SITE+1] = {-1, 16, 4, 7, 10, 13};
 int successeur[NB_SITE+1] = {-1, 6, 9, 12, 15, 3};
 */
int isvalueinarray(int val, int *arr, int size);


int cmpfunc (const void * a, const void * b)
{
    return ( *(int*)a - *(int*)b );
}

void simulateur(void) {
    
    
    int succMPI;
    int index, firstData;
    
    
    srand(time(NULL));   // should only be called once
    int i;
    
    for(i=1; i<=NB_SITE; i++){
        index = (rand() % (1 << K));
        while(isvalueinarray(index, cles, NB_SITE+1)){
            index = (rand() % (1 << K));
        }
        cles[i] = index;
    }
    qsort(cles, NB_SITE+1, sizeof(int), cmpfunc);
    
    for(i=1; i<=NB_SITE; i++){
        //printf("Sending ClesCHORD\n");
        MPI_Send(&cles[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);    // ENVOI DE LA CLE DU PAIR
        
        //printf("Sending firstData\n");
        if (i != 1){
            firstData = (cles[i-1] + 1);
            MPI_Send(&firstData, 1, MPI_INT, i, INIT, MPI_COMM_WORLD);  // ENVOIE DE LA PREMIERE DONNEE
            
        }else{
            firstData = (cles[NB_SITE] + 1) % (1 << K);
            MPI_Send(&firstData, 1, MPI_INT, 1, INIT, MPI_COMM_WORLD);  // ENVOIE DE LA PREMIERE DONNEE AU PREMIER SITE
        }
        
        //printf("Sending succCHORD\n");
        if (i != NB_SITE){
            successeur[i] = (cles[i+1]);
            MPI_Send(&successeur[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);  // ENVOIE DU SUCCESSEUR
        }else{
            successeur[NB_SITE] = (cles[1]);
            MPI_Send(&successeur[i], 1, MPI_INT, NB_SITE, INIT, MPI_COMM_WORLD);  // ENVOIE DU SUCCESSEUR
        }
        //printf("Sending succMPI\n");
        
        succMPI = (i)%NB_SITE+1;
        MPI_Send(&succMPI, 1, MPI_INT, i, INIT, MPI_COMM_WORLD);  // ENVOIE DU SUCCESSEUR MPI
    }
    
}


/******************************************************************************/
//  retourne 1 si k E ]a, b]
int app (int k, int a, int b){
    if (a < b) return ( (k > a) && (k <= b) );
    return ( (k > a) || (k <= b) );
}



/******************************************************************************/
// Lors de sa deconnexion, le processus D emet un message à son successeur.
// Le successeur S prend en charge les données que le proc D avait à charge, puis propage un message.
// Ce message se propage jusqu'au predecesseur P, qui doit alors changer de successeur pour pointer vers P.
void disconnect(int monRang, int succMPI, int firstData, int succCHORD){
    
    int aEnvoyer[4];
    
    aEnvoyer[0] = monRang; // necessaire pour que le predecessuer sache qu'il doit modifier son successeur
    aEnvoyer[1] = succMPI; // necessaire pour le predeceseur
    aEnvoyer[2] = firstData; //
    aEnvoyer[3] = succCHORD; //
    
    MPI_Send(&aEnvoyer, 4, MPI_INT, succMPI, DISCONNECT, MPI_COMM_WORLD);  // ENVOIE SIGNAL DECO
    printf("	DSC: envoie %d à %d (%d)\n", monRang, succCHORD, succMPI);
}


/******************************************************************************/

void lookup(int data, int init, int key, int monRang, int succ, int succMPI){
    //int finger;
    int aEnvoyer[2];
    
    aEnvoyer[0] = data;
    aEnvoyer[1] = init;
    
    
    sleep(1);
    
    if (app(data, succ, key)){
        MPI_Send(aEnvoyer, 2, MPI_INT, succMPI, LOOKUP, MPI_COMM_WORLD);  // ENVOIE DU SUCCESSEUR MPI
        printf("	FWD: je suis %d (%d), je forward la req a %d\n", key, monRang, succ);
    }
    else{
        MPI_Send(aEnvoyer, 2, MPI_INT, succMPI, LASTCHANCE, MPI_COMM_WORLD);  // ENVOIE DU SUCCESSEUR MPI
        printf("	LST: je suis %d (%d), je forward la lastChance a %d\n", key, monRang, succ);
    }
}

/******************************************************************************/

void chord(int rang){
    int key;
    int succ, succ_MPI;
    int firstData;
    MPI_Status status;
    
    char firstRequest = 1;
    //char stillAlive = 1;
    
    int dataToLookup = 0;
    
    int rcv[4];
    
    //printf("***INIT***\n");
    MPI_Recv(&key, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
    MPI_Recv(&firstData, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
    MPI_Recv(&succ, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
    
    MPI_Recv(&succ_MPI, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
    
    sleep(rang);
    printf("	INIT: proc %d (%d), firstData = %d, successeur = %d (%d)\n", key, rang, firstData, succ, succ_MPI);
    sleep(NB_SITE-rang);
    // FIN INIT
    
    sleep(1);
    while(1){
        
        if (rang == 1 && firstRequest){
            dataToLookup = 13;
            printf("\n	REQ: je suis %d (%d), je cherche la donnee %d\n", key, rang, dataToLookup);
            
            lookup(dataToLookup, rang, key, rang, succ, succ_MPI);
            firstRequest = 0;
            
            
        }
        
        else{
            
            MPI_Recv(&rcv, 4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            switch(status.MPI_TAG){
                case (LOOKUP):
                    lookup(rcv[0], rcv[1], key, rang, succ, succ_MPI);
                    break;
                case (ANSWER):
                    sleep(1);
                    printf("	ACK: je suis %d (%d), j'ai recu la donnee => %d\n", key, rang, rcv[0]);
                    sleep(3);	// un processus se deconnecte après avoir recu une donnee
                    printf("\n	DSC: je suis %d (%d), je me deconnecte, mon succ. etait %d\n", key, rang, succ);
                    disconnect(rang, succ_MPI, firstData, succ);
                    break;
                case (LASTCHANCE):
                    sleep(1);
                    printf("	RSP: je suis %d (%d), j'ai la donnee, je renvoie à %d\n", key, rang, rcv[1]);
                    rcv[0] = key;
                    MPI_Send(rcv, 2, MPI_INT, rcv[1], ANSWER, MPI_COMM_WORLD);  // ENVOIE REPONSE
                    break;
                case (DISCONNECT): // PREMIER MESSAGE DE DECO
                    sleep(1);
                    printf("	MAJ: je suis %d (%d), ma firstData est desormais %d\n", key, rang, rcv[2]);
                    firstData = rcv[2];
                    
                    if (rcv[0] == succ_MPI){
                        printf("	MAJ: je suis %d (%d), c'est mon succ qui se deco\n", key, rang);
                        printf("	MAJ: je suis %d (%d), mon nouveau succ est %d (%d)\n", key, rang, rcv[3], rcv[1]);
                        succ_MPI = rcv[1];
                        succ = rcv[3];
                    }
                    else{
                        printf("	FWD: je suis %d (%d), je propage\n", key, rang);
                        MPI_Send(&rcv, 4, MPI_INT, succ_MPI, DISCONNECT1, MPI_COMM_WORLD);
                    }
                    break;
                case (DISCONNECT1): // PROPAGATION MESSAGE DECO
                    sleep(1);
                    if (rcv[0] == succ_MPI){
                        printf("	MAJ: je suis %d (%d), c'est mon succ qui se deco\n", key, rang);
                        printf("	MAJ: je suis %d (%d), mon nouveau succ est %d (%d)\n", key, rang, rcv[3], rcv[1]);
                        succ_MPI = rcv[1];
                        succ = rcv[3];
                        
                        dataToLookup = rand() % (1 << K);
                        // le predecesseur du processus venant de se deconnecter fait une requete
                        printf("\n	REQ: je suis %d (%d), je cherche la donnee %d\n", key, rang, dataToLookup);
                        lookup(dataToLookup, rang, key, rang, succ, succ_MPI);
                        
                    }
                    else{
                        printf("	FWD: je suis %d (%d), je propage\n", key, rang);
                        MPI_Send(&rcv, 4, MPI_INT, succ_MPI, DISCONNECT1, MPI_COMM_WORLD);
                    }
                    break;
            }
        }
        
        
    }
}

/******************************************************************************/
// Cette fonction retourne 1 si la valeur recherchée est dans le tableau
int isvalueinarray(int val, int *arr, int size){
    int i;
    for (i=0; i < size; i++) {
        if (arr[i] == val)
        return 1;
    }
    return 0;
}

/******************************************************************************/

int main (int argc, char* argv[]) {
    int nb_proc,rang;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
    
    if (nb_proc != NB_SITE+1) {
        printf("Nombre de processus incorrect !\n");
        MPI_Finalize();
        exit(2);
    }
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rang);
    
    if (rang == 0) {
        simulateur();
    } else {
        chord(rang);
    }
    
    MPI_Finalize();
    return 0;
}

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>    // time()

#define INIT 100

#define LOOKUP 101
#define LASTCHANCE 102
#define ANSWER 103

#define CONNECT 104
#define DISCONNECT 105
#define DISCONNECT1 106

#define NB_SITE 16
#define K 8


#define nb_bits_identif 8



/*

AUTEURS: CHABAUX Paul / WANG Zengzhe

Compilation :  mpicc -o chord2 chord2.c
Execution	:  mpirun -np 17 chord2

Le nombre de site est modifiable

Les clés CHORD sont choisis aléatoirement par le proc0 dans la fonction simulateur. 

Dans ce programme, le proc0 va initialiser les variables locales de chaque proc.
Ensuite, le proc1 va chercher une donnée. Puis, le proc responsable de cette donnée va faire une seconde recherche, etc.
Les valeurs des données recherchées sont aléatoires.

Le scénario ne s'arrete donc pas.

Le programme MPI ne s'arrete pas proprement, car les processus écoutent encore les requetes des autres.
Ce programme utilise beaucoup la fonction sleep() afin de ralentir son execution et d'avoir une trace lisible et ordonée.
*/

int indexOfValue(int val, int *arr, int size);
int isvalueinarray(int val, int *arr, int size);

int cmpfunc (const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}



// cette fonction initie les variables locales de chaque processus (fingertable, identifiant, ...)

void simulateur(void) {
	int cles[NB_SITE+1] = {-1};
	int premiereDonnee[NB_SITE+1] = {-1};

	int fingerTable[K];
	int fingerMPI[K];
	int i, j, index, firstData;
	
	srand(time(NULL));   // should only be called once
	
	for(i=1; i<=NB_SITE; i++){
		index = (rand() % (1 << K));
		while(isvalueinarray(index, cles, NB_SITE+1)){ // calcul clés aléatoires
			index = (rand() % (1 << K));      
		}
		cles[i] = index;
	}
	qsort(cles, NB_SITE+1, sizeof(int), cmpfunc);	// on tri le tableau des clés (quicksort)


	for(i=1; i<=NB_SITE; i++){
		printf("Sending ClesCHORD\n");
		MPI_Send(&cles[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);    // ENVOI: clé CHORD
		
        printf("Sending firstData\n");
		if (i != 1){
			firstData = (cles[i-1] + 1);
			MPI_Send(&firstData, 1, MPI_INT, i, INIT, MPI_COMM_WORLD);  // ENVOIE DE LA PREMIERE DONNEE
			
		}else{
			firstData = (cles[NB_SITE] + 1) % (1 << K);
			MPI_Send(&firstData, 1, MPI_INT, 1, INIT, MPI_COMM_WORLD);  // ENVOIE DE LA PREMIERE DONNEE AU PREMIER SITE
		}

		// CALCUL FINGER TABLES
		for(j = 0; j < K; j++){
			fingerTable[j] = (cles[i] + (1 << j)) % (1 << K);			 // calcul finger ideal (cle + 2^j) % 2^k
			printf("fingerIdeal[%d][%d] = %d\n", cles[i], j, fingerTable[j]);
			
			while (!isvalueinarray(fingerTable[j], cles, NB_SITE+1))	// trouver finger reel dans les clés CHORD
				fingerTable[j] = (fingerTable[j]+1)%(1<<K);

			fingerMPI[j] = indexOfValue(fingerTable[j], cles, NB_SITE+1);  // trouver indexMPI du finger reel
			printf("fingerReal[%d][%d] = %d (%d)\n\n", cles[i], j, fingerTable[j], fingerMPI[j]);
			
		}
		MPI_Send(fingerTable, K, MPI_INT, i, INIT, MPI_COMM_WORLD); // ENVOIE DES FINGER TABLES
		MPI_Send(fingerMPI, K, MPI_INT, i, INIT, MPI_COMM_WORLD); // ENVOIE DES FINGER TABLES MPI

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

int indexOfValue(int val, int *arr, int size){
    int i;
    for (i=0; i < size; i++) {
        if (arr[i] == val)
            return i;
    }
    return -1;
}



/******************************************************************************/


//  retourne 1 si ]a, b]
int app (int k, int a, int b){
	if (a < b) return ( (k > a) && (k <= b) );
	return ( (k > a) || (k <= b) );
}

/* [a, b[

int app (int k, int a, int b){
	if (a < b) return ( (k >= a) && (k < b) );
	return ( (k >= a) || (k < b) );
}

*/



/******************************************************************************/
// 
void lookup(int data, int init, int key, int monRang, int* fingers, int* fingersMPI){
	//int finger;
	int aEnvoyer[2];

	aEnvoyer[0] = data;
	aEnvoyer[1] = init;

	
	sleep(1);
	int i = K-1;
	while(i != 0){
		if (app(data, fingers[i], key)){
		MPI_Send(aEnvoyer, 2, MPI_INT, fingersMPI[i], LOOKUP, MPI_COMM_WORLD);  // ENVOIE AU FINGER 5
		printf("	FWD: je suis %d (%d), je forward la req a %d, mon %d° finger\n", key, monRang, fingers[i], i);
		return;
		}
		i--;
	}
	
	MPI_Send(aEnvoyer, 2, MPI_INT, fingersMPI[0], LASTCHANCE, MPI_COMM_WORLD);  // ENVOIE LASTCHANCE AU FINGER 0
	printf("	LST: je suis %d (%d), je forward la lastChance a %d, mon premier finger\n", key, monRang, fingers[0]);
}

/******************************************************************************/

void chord(int rang){
	int key;
	int firstData, i;
	int finger[K] = {1};
	int fingerMPI[K] = {1};
	MPI_Status status;
	
	char firstRequest = 1;
	int dataToLookup = 0;
	
	int rcv[2];
	
	// DEBUT INIT
	
	MPI_Recv(&key, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&firstData, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
	MPI_Recv(finger, K, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
	MPI_Recv(fingerMPI, K, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
	

	sleep(rang);
	printf("INIT: proc %d (%d), 	firstData = %d, 	finger = {", rang, key, firstData);
	for (i = 0; i < K-1; i++)
		printf("%d, ", finger[i]);
	printf("%d}\n", finger[K-1]);
	sleep(NB_SITE-rang+1);
	
 

	// FIN INIT


	while(1){

		if (rang == 1 && firstRequest){		// le proc1 cherche une data - PREMIERE RECHERCHE
			dataToLookup = rand() % (1 << K);
			printf("\n\n	REQ: je suis %d (%d), je cherche la donnee %d\n", key, rang, dataToLookup);

			lookup(dataToLookup, rang, key, rang, finger, fingerMPI);
			firstRequest = 0;
		}

		else{

			MPI_Recv(&rcv, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // Ecoute
			switch(status.MPI_TAG){
				case (LOOKUP):	// on recoit un forward
					lookup(rcv[0], rcv[1], key, rang, finger, fingerMPI);
					break;
				case (ANSWER): // forward
					sleep(1);
					printf("	FWD: je suis %d (%d), j'ai la donnee => %d\n", key, rang, rcv[0]);
					break;
				case (LASTCHANCE): // on est responsable de la donnée
					sleep(1);
					printf("	RSP: je suis %d (%d), j'ai la donnee, je renvoie à %d\n", key, rang, rcv[1]);
					rcv[0] = key;
					MPI_Send(rcv, 2, MPI_INT, rcv[1], ANSWER, MPI_COMM_WORLD);  // ENVOIE REPONSE
					
					sleep(3); // NOUVELLE REQUETE
					dataToLookup = rand() % (1 << K);
					printf("\n\n	REQ: je suis %d (%d), je cherche la donnee %d\n", key, rang, dataToLookup);
					lookup(dataToLookup, rang, key, rang, finger, fingerMPI);
					break;
			}		
		}
	}
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

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define INIT 100
#define LOOKUP 101
#define LASTCHANCE 102
#define ANSWER 103
#define CONNECT 104
#define DISCONNECT 105

#define NB_SITE 5
#define nb_bits_identif 8

void simulateur(void) {

	int cles[NB_SITE+1] = {-1, 3, 6, 9, 12, 15}; //A MODIF EN TIRAGE ALEATOIRE
	int premiereDonnee[NB_SITE+1] = {-1, 16, 4, 7, 10, 13};
	int MPI[NB_SITE+1,2] = {{-1,-1},{3,0}, {6,1}, {9,2}, {12,3}, {15,4}};
        //int successeur[NB_SITE+1] = {-1, 6, 9, 12, 15, 3};
	int succMPI;
	

	int i;
	for(i=1; i<=NB_SITE; i++){
		MPI_Send(&cles[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);    // ENVOI DE LA CLE DU PAIR
        MPI_Send(&premiereDonnee[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);  // ENVOIE DE LA PREMIERE DONNEE
        //MPI_Send(&successeur[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);  // ENVOIE DU SUCCESSEUR
        MPI_Send(&MPI[i], 2, MPI_INT, i, INIT, MPI_COMM_WORLD);  // ENVOIE DU SUCCESSEUR

		succMPI = (i)%NB_SITE+1;
        MPI_Send(&succMPI, 1, MPI_INT, i, INIT, MPI_COMM_WORLD);  // ENVOIE DU SUCCESSEUR MPI
	}
}


/******************************************************************************/
// ]a, b]
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

void disconnect(){
	MPI_Send(rcv, 2, MPI_INT, rcv[1], DISCONNECT1, MPI_COMM_WORLD);  // ENVOIE REPONSE
}	


/******************************************************************************/

void lookup(int data, int init, int key, int monRang, int succ,int succMPI){
	int finger;
	int aEnvoyer[2];

	aEnvoyer[0] = data;
	aEnvoyer[1] = init;

	
	sleep(1);

	if (app(data, succ, key)){
		MPI_Send(aEnvoyer, 2, MPI_INT, succMPI, LOOKUP, MPI_COMM_WORLD);  // ENVOIE DU SUCCESSEUR MPI
		printf("\n	je suis %d (%d), je forward la req a %d\n", key, monRang, succ);
	}
	else{
		MPI_Send(aEnvoyer, 2, MPI_INT, succMPI, LASTCHANCE, MPI_COMM_WORLD);  // ENVOIE DU SUCCESSEUR MPI
		printf("\n	je suis %d (%d), je forward la lastChance a %d\n", key, monRang, succ);
	}	
}

/******************************************************************************/

void chord(int rang){
	int key;
	//int succ, succ_MPI;
        int rmpi[NB_SITE+1,2];
	int firstData;
	MPI_Status status;

	int dataToLookup = 0;

	int rcv[2];

	printf("***INIT***");
	MPI_Recv(&key, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&firstData, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
	//MPI_Recv(&succ, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
        //MPI_Recv(&succ, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
        MPI_Recv(&rmpi, 2, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&succ_MPI, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
	//printf("	je suis le proc %d, clé est %d, premiere donnee est %d, successeur est %d (%d)\n", rang, key, firstData, succ, succ_MPI);
        printf("	je suis le proc %d, clé est %d, premiere donnee est %d", rang, key, firstData);
	sleep(1);  

	if (rang == 1){		
		dataToLookup = 186;
		printf("\n	je suis %d (%d), je cherche la donnee %d\n", key, rang, dataToLookup);

		lookup(dataToLookup, rang, key, rang, rmpi[monRang+1,1], succ_MPI);
		MPI_Recv(&rcv, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("	je suis %d (%d), j'ai la donnee => %d\n", key, rang, rcv[0]);
		
	}
	else{
		MPI_Recv(&rcv, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		switch(status.MPI_TAG){
			case (LOOKUP):
				lookup(rcv[0], rcv[1], key, rang, rmpi[monRang+1,1]S, succ_MPI);
				break;
			case (ANSWER):
				sleep(1);
				printf("	je suis %d (%d), j'ai la donnee => %d\n", key, rang, rcv[0]);
				break;
			case (LASTCHANCE):
				sleep(1);
				printf("	je suis %d (%d), j'ai la donnee, je renvoie à %d\n", key, rang, rcv[1]);
				rcv[0] = key;
				MPI_Send(rcv, 2, MPI_INT, rcv[1], ANSWER, MPI_COMM_WORLD);  // ENVOIE REPONSE
				break;
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

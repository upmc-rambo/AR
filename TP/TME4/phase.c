#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define TAGINIT    0
#define TAGMESS    1
#define NB_SITE 6

#define DIAMETRE 5

void simulateur(void) {
   int i;

   /* nb_voisins_in[i] est le nombre de voisins entrants du site i */
   /* nb_voisins_out[i] est le nombre de voisins sortants du site i */
   int nb_voisins_in[NB_SITE+1] = {-1, 2, 1, 1, 2, 1, 1};
   int nb_voisins_out[NB_SITE+1] = {-1, 2, 1, 1, 1, 2, 1};

   int min_local[NB_SITE+1] = {-1, 4, 7, 11, 6, 12, 9};

   /* liste des voisins entrants */
   int voisins_in[NB_SITE+1][2] = {{-1, -1}, 
				   {4, 5}, {1, -1}, {1, -1}, // voisins vers 1 = 4 & 5...
				{3, 5}, {6, -1}, {2, -1}};
                               
   /* liste des voisins sortants */
   int voisins_out[NB_SITE+1][2] = {{-1, -1},
				{2, 3}, {6, -1}, {4, -1},
				{1, -1}, {1, 4}, {5,-1}};

   for(i=1; i<=NB_SITE; i++){
     MPI_Send(&nb_voisins_in[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD); // envoie à i son nb de voisins entrants
     MPI_Send(&nb_voisins_out[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD); // envoie à i son nb voisins sortants
     MPI_Send(voisins_in[i], nb_voisins_in[i], MPI_INT, i, TAGINIT, MPI_COMM_WORLD);   // envoie à i ses voisins entrants
     MPI_Send(voisins_out[i], nb_voisins_out[i], MPI_INT, i, TAGINIT, MPI_COMM_WORLD); // envoie à i ses voisins sortants
     MPI_Send(&min_local[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD);  // envoie à i son min local.
   }
}

/******************************************************************************/


void calcul_min(int rang){

  short nonDecid = 1;
  int nbVoisIn, nbVoisOut, monMin, i, minCourant, sCount, minRecu;
  MPI_Status status;
  
  short Dp = 0; // decide
  short Sp = 0; // send
  sCount = 0;
  
  // recep nb Voisin vers moi
  MPI_Recv(&nbVoisIn, 1, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);
  int voisins_in[nbVoisIn];  
  int rCount[nbVoisIn];

  for (i = 0; i < nbVoisIn; ++i){
    rCount[i] = 0;
  }

  //recep nb Voisins ou je peux emmetre
  MPI_Recv(&nbVoisOut, 1, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);
  int voisins_out[nbVoisOut];

  MPI_Recv(&voisins_in, nbVoisIn, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status); // On recoit nbVoisIn

  MPI_Recv(&voisins_out, nbVoisOut, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);//On recoit nbVoisOut
  MPI_Recv(&monMin, 1, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);
  minCourant = monMin;

  //printf("je suis le proc %d, j'ai %d voisin In, %d out, mon min local est %d\n", rang, nbVoisIn, nbVoisOut, monMin);

  /*--------------------------------FIN TOPOLOGIE----------------------------------------*/
  
  while (nonDecid){
    
    // decide condition update
    Dp = 1;
    for (i = 0; i < nbVoisIn && Dp == 1; ++i){
      if (rCount[i] < DIAMETRE) Dp = 0;
    }
    // send condition update
    Sp = 1;
    for (i = 0; i < nbVoisIn && Sp == 1; ++i){
      if (rCount[i] < sCount || sCount >= DIAMETRE) Sp = 0;
    }
    
    
    if (Dp){ // Decide process
      printf("je suis le proc %d, je DECIDE -> LE MIN EST %d\n", rang, minCourant);
      nonDecid = 0;
    }
    else if (Sp){ // Send process
      printf("je suis le proc %d, j'envoie à tout mes voisins %d\n", rang, minCourant);
      for (i = 0; i < nbVoisOut; i++){
	MPI_Send(&minCourant, 1, MPI_INT, voisins_out[i], TAGMESS, MPI_COMM_WORLD);
      }
      sCount++;
    }
    else{ // Receive process
     
      MPI_Recv(&minRecu, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      for (i = 0; i < nbVoisIn; i++){
	if (voisins_in[i] == status.MPI_SOURCE) break;
      }
      if (minRecu < minCourant) minCourant = minRecu;      
      printf("je suis le proc %d, j'ai recu %d du proc %d (min courant=%d)\n", rang, minRecu, status.MPI_SOURCE, minCourant);
      rCount[i]++;
    }    
  }
  
}


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
      calcul_min(rang);
   }
  
   MPI_Finalize();
   return 0;
}

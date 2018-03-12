#include <stdio.h>
#include <string.h>
#include <mpi.h>
#define MASTER 0
#define SIZE 128
int main(int argc, char **argv){
	int my_rank;
	int nb_proc;
	int source;
	int dest;
	int tag =0;
	char message[SIZE];
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);


	printf("Processus %d sur %d : Hello MPI\n",my_rank, nb_proc);

	MPI_Finalize();
	return 0;
}

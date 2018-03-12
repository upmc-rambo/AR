#ifndef MPI_SERVER
#define MPI_SERVER
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <mpi.h>




typedef struct server{
	pthread_t listener;
	pthread_mutex_t mutex;
	void (*callback)(int tag, int source); //procedure de reception de message
} server;



void start_server(void (*callback)(int tag, int source)); /*initialiser le serveur*/
void destroy_server();/*detruire le serveur*/
pthread_mutex_t* getMutex();/*renvoyer une reference sur le mutex*/
#endif

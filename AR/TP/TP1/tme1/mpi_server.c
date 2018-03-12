#include "mpi_server.h"


static server the_server;





//ICI CODE THREAD SERVEUR A COMPLETER
void start_server(void (*callback)(int tag, int source))
{
//A COMPLETER
}


void destroy_server()
{
//A COMPLETER
}


pthread_mutex_t* getMutex()
{
	pthread_mutex_lock(&the_server.mutex);
	return &the_server.mutex;
}

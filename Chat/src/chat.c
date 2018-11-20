#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include "server.h"

pthread_t serv_thread;

int
main (int argc, char *argv[]) {
	if (argc != 2 ) {
		printf("usage: ./chat ip_addr\n");
		return 1;
	}

	char * ip_addr = argv[1];

	pthread_create(&serv_thread, NULL, server_thread, NULL);

	pthread_join(serv_thread, NULL);
	return 0;
}

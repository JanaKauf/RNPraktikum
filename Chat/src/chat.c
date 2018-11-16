#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

pthread_t active;
pthread_t passive;

void *
active_thread (void * args) {
	printf("## active_thread started\n");
}

void *
passive_thread (void * args) {
	printf("## passive_thread started\n");
}


int
main (int argc, char *argv[]) {
	
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include "server.h"
#include "taskqueue.h"
#include "thpool.h"

pthread_t serv_thread;
mqd_t task_queue;
struct threadpool * pool;


void *
cmd_thread (void *args ) {
	char id;
	char msg[1024];
	char name_id[16];
	char cmd[10];
	char ip_addr[16];

	while(1) {
		fflush(stdin);
		id = (char) getc(stdin);

		if (id == '@') {
			fgets(name_id, 16, stdin);
			fgets(msg, 1024, stdin);
		
		} else if(id == '/') {
			fgets(cmd, 10, stdin);
			if (strcmp(cmd, "connect") == 0) {
				fgets(ip_addr, 16, stdin);
			
			} else if (strcmp(cmd, "quit") == 0) {
			
			} else if (strcmp(cmd, "info") == 0) {
			
			} else {
				printf("usage.... ");
			}
		
		} else {
			printf("usage....");
		}
		
	}
}

int
main (int argc, char *argv[]) {

	task_queue = taskqueue_create("TASK_QUEUE", 20);
	if ((pool = thpool_create(task_queue)) == NULL) {
		taskqueue_destroy("TASK_QUEUE");
		return 0;
	}

	if ((errno = server_init()) != 0) {
		thpool_destroy(pool);
		thpool_free(pool);
		return 1;
	}
	pthread_create(&serv_thread, NULL, server_thread, pool);

	pthread_join(serv_thread, NULL);
	return 0;
}

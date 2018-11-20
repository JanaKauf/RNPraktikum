#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include "server.h"
#include "taskqueue.h"
#include "thpool.h"
#include "task.h"

pthread_t serv_thread;
mqd_t task_queue;
struct threadpool * pool;


void *
cmd_thread (void *args ) {
	char msg[1024];
	char space[] = " ";
	char *cmd;

	struct task job;

	while(1) {
		fflush(stdin);
		fgets(msg, 1024, stdin);

		job.arg = msg;

		if (msg[0] == '@') {
			job.routine_for_task = send_msg;
			thpool_add_task(pool, job, 1);
		
		} else {

			cmd = strtok(msg, space);

			if (strcmp(cmd, "/connect") == 0) {
				job.routine_for_task = send_sign_in;
				thpool_add_task(pool, job, 1);
		
			} else if (strcmp(cmd, "/quit") == 0) {
				job.routine_for_task = send_quit;
				thpool_add_task(pool, job, 1);
		
			} else if (strcmp(cmd, "/info") == 0){
			
			} else if (strcmp(cmd, "/help") == 0){
			
			} else {
				continue;
			}
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

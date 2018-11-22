#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include "server.h"
#include "taskqueue.h"
#include "thpool.h"
#include "task.h"
#include "list.h"

pthread_t serv_thread;

pthread_t cmd_control;

mqd_t task_recv;
mqd_t task_send;
mqd_t task_connection;

struct threadpool * recv_pool;
struct threadpool * send_pool;
struct threadpool * connection_pool;

void
help_function (void) {
	printf("# /connect <ip-adress>\t\t-- Connects to another peer\n");
	printf("# /quit\t\t-- Quits the program\n");
	printf("# /info\t\t-- Displays list members\n");
	printf("# /help\t\t-- Displays commands\n");

}

void *
cmd_routine (void *args ) {
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
			thpool_add_task(send_pool, job, 1);
		
		} else {

			cmd = strtok(msg, space);

			if (strcmp(cmd, "/connect") == 0) {
				job.routine_for_task = send_sign_in;
				thpool_add_task(send_pool, job, 1);
		
			} else if (strcmp(cmd, "/quit") == 0) {
				job.routine_for_task = send_quit;
				thpool_add_task(send_pool, job, 1);
		
			} else if (strcmp(cmd, "/info") == 0){
				print_members();
			
			} else if (strcmp(cmd, "/help") == 0){
				help_function();	
			} else {
				continue;
			}
		}



			
	}
}

int
main (int argc, char *argv[]) {

	task_recv = taskqueue_create("TASK_RECV", 15);
	if ((recv_pool = thpool_create(task_recv)) == NULL) {
		taskqueue_destroy("TASK_QUEUE");
		return 0;
	}

	task_send = taskqueue_create("TASK_SEND", 15);
	if ((send_pool = thpool_create(task_send)) == NULL) {
		taskqueue_destroy("TASK_QUEUE");
		return 0;
	}

	task_connection = taskqueue_create("TASK_CONNECTION", 15);
	if ((connection_pool = thpool_create(task_connection)) == NULL) {
		taskqueue_destroy("TASK_QUEUE");
		return 0;
	}

	pthread_create(&serv_thread, NULL, server_thread, recv_pool);
	pthread_create(&cmd_control, NULL, cmd_routine, NULL);

	pthread_join(cmd_control, NULL);
	return 0;
}

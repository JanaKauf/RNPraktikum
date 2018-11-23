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
#include "list_thrsafe.h"

pthread_t serv_thread;
pthread_t cmd_control;

mqd_t task_recv;
mqd_t task_send;
mqd_t task_connection;

struct threadpool *recv_pool;
struct threadpool *send_pool;

void
help_function (void) {
	printf("# /connect <ip-adress>\t\t-- Connects to another peer\n");
	printf("# /quit\t\t\t\t-- Quits the program\n");
	printf("# /info\t\t\t\t-- Displays list members\n");
	printf("# /help\t\t\t\t-- Displays commands\n");

}

void *
Cmd_routine (void *args ) {
	char msg[1024] = "";
	char *cmd = "";

	struct task job;

	pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_DEFERRED, NULL);
	while(1) {
		memset(msg, 0, 1024);
//		get_string(1, msg);
		fflush(stdin);
		fgets(msg, 1024, stdin);

		job.arg = msg;

		if (strncmp(msg, "", 1) == 0) {
			continue;
		}

		if (msg[0] == '@') {
			job.routine_for_task = send_msg;
			Thpool_add_task(send_pool, job, 1);
		
		} else {

			cmd = strtok(msg, " ");

			if (strncmp(cmd, "/connect", 8) == 0) {
				job.arg = strtok(NULL, "\n");
				job.routine_for_task = send_sign_in;
				Thpool_add_task(send_pool, job, 1);
		
			} else if (strncmp(cmd, "/quit", 5) == 0) {
				//job.routine_for_task = send_quit;
				//Thpool_add_task(send_pool, job, 1);
				pthread_exit(0);
				break;
		
			} else if (strncmp(cmd, "/info", 5) == 0){
				List_print();
			
			} else if (strncmp(cmd, "/help", 5) == 0){
				help_function();
			} else {
				continue;
			}
		}
			
	}
	return NULL;
}

int
main (int argc, char *argv[]) {

	printf("-- Creating Task Queues...\n");

	task_recv = Taskqueue_create("/TASK_RECV", NUM_TASKS);
	task_send = Taskqueue_create("/TASK_SEND", NUM_TASKS);
	task_connection = Taskqueue_create("/TASK_CONNECTION", NUM_TASKS);

	printf("-- Creating member_list...\n");

	if (Thrsafe_init() != 0) {
		goto taskfree;	
	}

	printf("-- Creating thpool - RECV...\n");

	if ((recv_pool = Thpool_create(task_recv)) == NULL) {
		printf("main: recv_pool = create - fail");
		goto taskfree;
	}

	printf("-- Creating thpool - SEND...\n");

	if ((send_pool = Thpool_create(task_send)) == NULL) {
		printf("main: send_pool = create - fail");
		goto recvfree;
	}

	if (Thrsafe_init() != 0) {
		goto sendfree;
	}

	printf("-- Serv_thread create...\n");
	pthread_create(&serv_thread, NULL, Server_thread, recv_pool);

	printf("-- Cmd_thread create...\n");
	pthread_create(&cmd_control, NULL, Cmd_routine, NULL);

	pthread_join(cmd_control, NULL);
	pthread_cancel(serv_thread);
	pthread_join(serv_thread, NULL);

	Thrsafe_clean();

	sendfree:
		Thpool_destroy(send_pool);
		Thpool_free(send_pool);
	recvfree:
		Thpool_destroy(recv_pool);
		Thpool_free(recv_pool);
	taskfree:
		Taskqueue_destroy("/TASK_RECV");
		Taskqueue_destroy("/TASK_CONNECTION");
		Taskqueue_destroy("/TASK_SEND");
		
	return 0;

}


struct threadpool *
Chat_get_recvpool(void) {
	return recv_pool;
}

struct threadpool *
Chat_get_sendpool (void) {
	return send_pool;
}

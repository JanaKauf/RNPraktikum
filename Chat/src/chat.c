/***********************************************************************
 * Takes Input from User and puts tasks in the Task-Queue   		   *
 ***********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#include "server.h"
#include "client.h"
#include "thpool.h"
#include "task.h"
#include "list.h"
#include "color.h"

pthread_t serv_thread;
pthread_t cmd_control;

struct threadpool *recv_pool;
struct threadpool *send_pool;

void
help_function (void) {
	printf(MAG "\n# /connect <ip-adress>\t\t-- Connects to another peer\n");
	printf("# /quit\t\t\t\t-- Quits the program\n");
	printf("# /info\t\t\t\t-- Displays list members\n");
	printf("# /help\t\t\t\t-- Displays commands\n\n" RESET);

}

void *
Cmd_routine (void *args ) {
	printf(GRN "## CMD_thread started\n" RESET);
	char msg[1024] = "";
	char *cmd = "";

	struct task_t job_msg;
	struct task_t job_sign;

	char * ip;

	pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_DEFERRED, NULL);
	while(1) {
		memset(msg, 0, 1024);
		fflush(stdin);
		fgets(msg, 1024, stdin);

		if (strncmp(msg, " ", 1) == 0) {
			continue;
		}

		if (msg[0] == '@') {
			job_msg.routine_for_task = send_msg;
			job_msg.arg = malloc(sizeof(msg));
			strcpy(job_msg.arg, msg);

			Thpool_add_task(send_pool, job_msg);

			printf("Sending message...\n");
		
		} else {
			cmd = strtok(msg, " \n");

			if (strncmp(cmd, "/connect", 8) == 0) {
				ip = strtok(NULL, "\n");
				if(strlen(ip) < 7) {
					help_function();
					continue;
				}

				job_sign.routine_for_task = send_sign_in;
				job_sign.arg = malloc(sizeof(ip));
				strcpy(job_sign.arg, ip);

				Thpool_add_task(send_pool, job_sign);

				printf(MAG "Connecting to %s...\n" RESET, ip);
		
			} else if (strncmp(cmd, "/quit", 5) == 0) {
//				job_quit.routine_for_task = send_quit;
//				job_quit.arg = malloc(sizeof(msg));
//				job_quit.arg = msg;
//				Thpool_add_task(send_pool, job_quit);

				send_quit(msg);

				printf("Quiting...\n");
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

	if (argc != 3) {
		printf(RED "usage: ./chat <ID> <Protokoll>\n" RESET);
		return -1;
	
	}

	char ** arg;

	arg = argv;

	printf(MAG "-- Creating member_list...\n" RESET);

	if (Tasks_start() != 0) {
		return 0;
	}

	printf(MAG "-- Creating thpool - RECV...\n" RESET);

	if ((recv_pool = Thpool_create()) == NULL) {
		printf("main: recv_pool = create - fail");
		goto thrsafe;
	}

	printf(MAG "-- Creating thpool - SEND...\n" RESET);

	if ((send_pool = Thpool_create()) == NULL) {
		printf("main: send_pool = create - fail");
		goto recvfree;
	}

	Client_protocol(argv[2]);

	printf(MAG "-- Serv_thread create...\n" RESET);
	pthread_create(&serv_thread, NULL, Server_thread, arg);

	printf(MAG "-- Cmd_thread create...\n" RESET);
	pthread_create(&cmd_control, NULL, Cmd_routine, NULL);

	pthread_join(cmd_control, NULL);
	pthread_cancel(serv_thread);
	pthread_join(serv_thread, NULL);

	Thpool_destroy(send_pool);
	Thpool_free(send_pool);

	recvfree:
		Thpool_destroy(recv_pool);
		Thpool_free(recv_pool);
	thrsafe:
		Tasks_clean();
		
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

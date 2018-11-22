/*
 * server.c
 *
 *  Created on: Nov 16, 2018
 *      Author: networker
 */

#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "thpool.h"
#include "task.h"
#include "list_thrsafe.h"
#include "list.h"


int sockfd;
struct addrinfo hints;
struct sockaddr_storage their_addr;
int yes = 1;

int
server_init(void) {
	struct addrinfo *servlist, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((errno = getaddrinfo(0, PORT, &hints, &servlist)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errno));
		return 1;
	}

	for(p = servlist; p != NULL; p = p->ai_next) {
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))== -1) {
			perror("server: setsockopt");
			errno = EPERM;
			return errno;
		}

		if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	freeaddrinfo(servlist);

	if(p == NULL) {
		errno = EPERM;
		return errno;
	}


	if (thrsafe_set_socket_id("Raupe\0", &sockfd) != 0) {
		return -1;
	}

	if (list_set_first_ip() != 0) {
		return -1;
	}

	if(listen(sockfd, HOLD_QUEUE) == -1) {
		errno = EPERM;
		return errno;
	}

	printf(" | server is listening\t:::\n");


	return 0;
}

void *
server_thread (void * args) {
	int new_fd;
	struct sockaddr_storage client_addr;
	socklen_t addr_len;
	char client_ip[INET_ADDRSTRLEN];
	int num_bytes;
	char buf[1024];

	struct threadpool * pool = args;
	struct task job;

	uint8_t type;

	if (server_init() != 0) {
		return &errno;
	}

	printf("## server_thread started\n");

	for(;;) {
		
        addr_len = sizeof client_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);

        if (new_fd == -1) {
			perror("accept");
			continue;
		}

        printf("server_thread: new connection from %s on socket %d\n",
				inet_ntop(client_addr.ss_family, &(((struct sockaddr_in*)&client_addr)->sin_addr),
				client_ip, INET_ADDRSTRLEN), new_fd);

		job.routine_for_task = recv_from_client;
		job.arg = &new_fd;

		thpool_add_task(pool, job, 1);
		printf("Task added.");
	}

}

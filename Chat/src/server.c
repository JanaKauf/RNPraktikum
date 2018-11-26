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
#include <sys/ioctl.h>
#include <net/if.h>

#include "thpool.h"
#include "task.h"
#include "list.h"


int sock_fd;
struct addrinfo hints;
struct sockaddr_storage their_addr;
int yes = 1;

int
Server_init(void) {
	struct addrinfo *servlist, *p;

	char ip[INET_ADDRSTRLEN];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((errno = getaddrinfo(0, PORT, &hints, &servlist)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errno));
		return 1;
	}

	for(p = servlist; p != NULL; p = p->ai_next) {
		if((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))== -1) {
			perror("server: setsockopt");
			errno = EPERM;
			return errno;
		}

		if(bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock_fd);
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

	struct ifreq ifr;
	strcpy(ifr.ifr_name, INTERFACE_NAME);	

	if (ioctl(sock_fd, SIOCGIFADDR, &ifr) != 0) {
		perror("ioctl: ");
		return -1;
	}

	struct sockaddr_in * my_ip = (struct sockaddr_in *) &ifr.ifr_addr;

	if (List_init(ID_NAME, my_ip->sin_addr.s_addr) != 0) {
		return -1;
	}

	if(listen(sock_fd, HOLD_QUEUE) == -1) {
		errno = EPERM;
		return errno;
	}

	printf(" | server is listening\t:::\n");


	return 0;
}

void *
Server_thread (void *args) {
	int new_fd;
	struct sockaddr_storage client_addr;
	socklen_t addr_len;
	char client_ip[INET_ADDRSTRLEN];

	struct threadpool *pool = args;
	struct task_t job;

	if (Server_init() != 0) {
		return &errno;
	}

	printf("## server_thread started\n");

	pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_DEFERRED, NULL);
	while(1) {
		
        addr_len = sizeof client_addr;
        new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_len);

        if (new_fd == -1) {
			perror("accept");
			continue;
		}

        printf("server_thread: new connection from %s on sockfd %d\n",
				inet_ntop(client_addr.ss_family,
						&(((struct sockaddr_in*)&client_addr)->sin_addr), client_ip,
						INET_ADDRSTRLEN),
						new_fd);

		job.routine_for_task = recv_from_client;
		job.arg = &new_fd;

		Thpool_add_task(pool, job);
		printf("Task added.");
	}

}

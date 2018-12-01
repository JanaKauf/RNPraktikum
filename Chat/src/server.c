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
#include "color.h"
#include "chat.h"


int sock_server;
struct addrinfo hints;
struct sockaddr_storage their_addr;
int yes = 1;

int
Server_init(uint8_t * id, uint8_t * interface) {
	struct addrinfo *servlist, *p;


	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((errno = getaddrinfo(0, PORT, &hints, &servlist)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errno));
		return -1;
	}

	for(p = servlist; p != NULL; p = p->ai_next) {
		if((sock_server = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if(setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))== -1) {
			perror("server: setsockopt");
			return -1;
		}

		if(bind(sock_server, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock_server);
			perror("server: bind");
			continue;
		}
		break;
	}

	freeaddrinfo(servlist);

	if(p == NULL) {
		errno = EPERM;
		perror("Server_init: p");
		return -1;
	}

	struct ifreq ifr;
	strcpy(ifr.ifr_name, (char *)interface);

	if (ioctl(sock_server, SIOCGIFADDR, &ifr) != 0) {
		perror("ioctl: ");
		return -1;
	}

	struct sockaddr_in * my_ip = (struct sockaddr_in *) &ifr.ifr_addr;

	if (List_init(id, my_ip->sin_addr.s_addr) != 0) {
		return -1;
	}

	if(listen(sock_server, HOLD_QUEUE) == -1) {
		errno = EPERM;
		return -1;
	}

	printf(GRN " | server is listening\t:::\n" RESET);


	return 0;
}

void *
Server_thread (void *args) {
	int new_fd;
	struct sockaddr_storage client_addr;
	socklen_t addr_len;
	char client_ip[INET_ADDRSTRLEN];

	uint8_t * id = (uint8_t*)strtok((char*)args, " ");
	uint8_t * interface = (uint8_t*)strtok(NULL, "\0");

	struct threadpool *pool = Chat_get_recvpool();
	struct task_t job;

	if (Server_init(id, interface) != 0) {

		return NULL;
	}

	printf(GRN "## server_thread started\n" RESET);

	pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_DEFERRED, NULL);
	while(1) {
		
        addr_len = sizeof client_addr;
        new_fd = accept(sock_server, (struct sockaddr *)&client_addr, &addr_len);

        if (new_fd == -1) {
			perror("accept");
			continue;
		}

        printf(GRN "server_thread: new connection from %s on sockfd %d\n" RESET,
				inet_ntop(client_addr.ss_family,
						&(((struct sockaddr_in*)&client_addr)->sin_addr), client_ip,
						INET_ADDRSTRLEN),
						new_fd);

		job.routine_for_task = recv_from_client;
		job.arg = malloc(sizeof(int));
		*(int *)(job.arg) = new_fd;
		job.mallfree = true;

		Thpool_add_task(pool, job);
	}

}

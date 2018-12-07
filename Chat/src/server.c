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
Server_tcp_init(char * id, char * interface) {
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
		printf("%u\n", p->ai_addr);
		break;
	}

	freeaddrinfo(servlist);

	if(p == NULL) {
		errno = EPERM;
		perror("Server_init: p");
		return -1;
	}

	struct ifreq ifr;
	strcpy(ifr.ifr_name, interface);

	if (ioctl(sock_server, SIOCGIFADDR, &ifr) != 0) {
		perror("ioctl: ");
		return -1;
	}

	struct sockaddr_in * my_ip = (struct sockaddr_in *) &ifr.ifr_addr;

	printf("%u\n", my_ip->sin_addr.s_addr);

	if (List_init((uint8_t *)id, my_ip->sin_addr.s_addr) != 0) {
		return -1;
	}

	if(listen(sock_server, HOLD_QUEUE) == -1) {
		errno = EPERM;
		return -1;
	}

	printf(GRN " | server is listening\t:::\n" RESET);


	return 0;
}

int
Server_sctp_init(uint8_t *id, uint8_t *interface) {
	struct sockaddr_in sin[1];

	if((sock_server = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) == -1) {
		perror("server: socket");
		return -1;
	}

//	if(setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))== -1) {
//		perror("server: setsockopt");
//		return -1;
//	}

	sin->sin_family = AF_INET;
	sin->sin_port = 6100;
	sin->sin_addr.s_addr = INADDR_ANY;

	if(bind(sock_server, (struct sockaddr *)sin, sizeof(*sin)) == -1) {
		close(sock_server);
		perror("server: bind");
	}

	struct ifreq ifr;
	strcpy(ifr.ifr_name, interface);

	if (ioctl(sock_server, SIOCGIFADDR, &ifr) != 0) {
		perror("ioctl: ");
		return -1;
	}

	struct sockaddr_in * my_ip = (struct sockaddr_in *) &ifr.ifr_addr;

	if (List_init((uint8_t *)id, my_ip->sin_addr.s_addr) != 0) {
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

	char ** arg = args;

	char * id;
	char * interface;
	char * protocol;

	id = arg[1];
	interface = arg[2];
	protocol = arg[3];

	struct threadpool *pool = Chat_get_recvpool();
	struct task_t job;

	if (strncmp(protocol, "-sctp", 5) == 0) {
		if (Server_sctp_init(id, interface) != 0)
			pthread_exit(0);
	
	} else {
		if (Server_tcp_init(id, interface) != 0)
			pthread_exit(0);

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

		Thpool_add_task(pool, job);
	}

}

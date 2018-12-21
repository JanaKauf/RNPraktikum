/*
 * server.c
 *
 *  Created on: Nov 16, 2018
 *      Author: networker
 */

#include "server.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <sys/wait.h>
#include <net/if.h>
#include <linux/if_link.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>

#include "thpool.h"
#include "task.h"
#include "list.h"
#include "color.h"
#include "chat.h"


int sock_tcp_server;
int sock_sctp_server;
struct addrinfo hints;
struct sockaddr_storage their_addr;
int yes = 1;

int
Server_tcp_init(char * id) {
	printf("TCP\n");
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
		if((sock_tcp_server = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if(setsockopt(sock_tcp_server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))== -1) {
			perror("server: setsockopt");
			return -1;
		}

		if(bind(sock_tcp_server, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock_tcp_server);
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

	uint32_t ip;

	if(get_my_ip(&ip) == 0) {
		printf("couldnt find an interface..\n");
		return -1;
	}

	if (List_init((uint8_t *)id, ip) != 0) {
		return -1;
	}

	if(listen(sock_tcp_server, HOLD_QUEUE) == -1) {
		errno = EPERM;
		return -1;
	}

	printf(GRN " | server is listening\t:::\n" RESET);


	return 0;
}

int
Server_sctp_init(uint8_t *id) {
	printf("SCTP\n");
	struct sockaddr_in sin[1];

	struct sctp_paddrparams paddr;

	memset(&paddr, 0, sizeof(paddr));

	paddr.spp_address.ss_family = AF_INET;
	paddr.spp_flags = SPP_HB_ENABLE;
	paddr.spp_hbinterval = 15000;
	if((sock_sctp_server = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP)) == -1) {
		perror("server: socket");
		return -1;
	}

	sin->sin_family = AF_INET;
	sin->sin_port = htons(6100);
	sin->sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(sock_sctp_server, (struct sockaddr *)sin, sizeof(*sin)) == -1) {
		close(sock_sctp_server);
		perror("server: bind");
	}

	if(setsockopt(sock_sctp_server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))== -1) {
		perror("server: setsockopt");
		return -1;
	}

	int sso = setsockopt(sock_sctp_server, SOL_SOCKET, SCTP_PEER_ADDR_PARAMS, &paddr, sizeof(paddr));
	if (sso < 0) {
		perror("[inet] inet_create_socket: setsockopt() error");
		return -1;
	}

	uint32_t ip;

	if(get_my_ip(&ip) == 0) {
		printf("couldnt find an interface..\n");
		return -1;
	}

	if (List_init((uint8_t *)id, ip) != 0) {
		return -1;
	}


	if(listen(sock_sctp_server, HOLD_QUEUE) == -1) {
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
	char * protocol;

	id = arg[1];
	protocol = arg[2];

	struct threadpool *pool = Chat_get_recvpool();
	struct task_t job;

	if (strncmp(protocol, "-sctp", 5) == 0) {
		if (Server_sctp_init((uint8_t*)id) != 0)
			pthread_exit(0);
	
	} else {
		if (Server_tcp_init(id) != 0)
			pthread_exit(0);

	}

	printf(GRN "## server_thread started\n" RESET);

	pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_DEFERRED, NULL);
	while(1) {
		
        addr_len = sizeof client_addr;
        if (strncmp(protocol, "-sctp", 5) == 0) {
        	new_fd = accept(sock_sctp_server, (struct sockaddr *)&client_addr, &addr_len);
        } else {
        	new_fd = accept(sock_tcp_server, (struct sockaddr *)&client_addr, &addr_len);
        }

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


int
get_my_ip(uint32_t* ip) {
	struct ifaddrs *ifaddr, *ifa;
	int family, s;
	char host[NI_MAXHOST];
	int success = 0;

	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		return 0;
	}

	for (ifa = ifaddr; ifa != NULL && (success == 0); ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) {
			continue;
		}
		family = ifa->ifa_addr->sa_family;

		size_t size;
		if(family == AF_INET) {
			size = sizeof(struct sockaddr_in);
			s = getnameinfo(ifa->ifa_addr, size, host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if(s != 0) {
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
			}
			//TODO 10.16.x.x - 10.31.x.x not filtered
			if(strncmp(host, "127", 3) != 0 && strncmp(host, "141.22", 6) != 0 && strncmp(host, "10.", 3) != 0) {  //&& strncmp(host, "::1", 3) != 0 && strncmp(host, "fe80::", 6) != 0
				printf("\t\t%s \n", ifa->ifa_name);
				printf("\t\taddress: <%s>\n", host);

				*ip = ((struct sockaddr_in*)(ifa->ifa_addr))->sin_addr.s_addr;

				success = 1;
				continue;
			}
		}
	}
	freeifaddrs(ifaddr);
	return success;
}

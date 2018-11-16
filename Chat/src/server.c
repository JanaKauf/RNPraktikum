/*
 * server.c
 *
 *  Created on: Nov 16, 2018
 *      Author: networker
 */

#include "server.h"
#include <stdio.h>
#include <errno.h>
#define HOLD_QUEUE 20

int sockfd;
struct addrinfo hints;
struct sockaddr_storage their_addr;
int yes = 1;


int
server_start() {
	struct addrinfo *servinfo, *p;
	int rv;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((rv = getaddrinfo(0, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		errno = EPERM;
		return errno;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
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

	freeaddrinfo(&servinfo);
	if(p == NULL) {
		errno = EPERM;
		return errno;
	}


	if(listen(sockfd, HOLD_QUEUE) == -1) {
		errno = EPERM;
		return errno;
	}
	printf("server is listening...\n");



	return 0;
}



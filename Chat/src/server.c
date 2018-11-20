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


int sockfd;
struct addrinfo hints;
struct sockaddr_storage their_addr;
int yes = 1;

fd_set master;
fd_set read_fds;
int fd_max;

void *
get_in_addr(struct sockaddr *sa) {

	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int
server_init(void) {
	struct addrinfo *servlist, *p;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
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


	if(listen(sockfd, HOLD_QUEUE) == -1) {
		errno = EPERM;
		return errno;
	}

	printf(" | server is listening\t:::\n");

	FD_SET(sockfd, &master);
	fd_max = sockfd;

	return 0;
}

void *
server_thread (void * args) {
	int new_fd;
	struct sockaddr_storage client_addr;
	socklen_t addr_len;
	char client_ip[INET6_ADDRSTRLEN];
	int num_bytes;
	char buf[1024];


	printf("## server_thread started\n");

	while (1) {
		read_fds = master;
        if (select(fd_max+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        for(int i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == sockfd) {
                    addr_len = sizeof client_addr;
                    new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);

                    if (new_fd == -1) {
                        perror("accept");

                    } else {
                        FD_SET(new_fd, &master);

                        if (new_fd > fd_max) {
                            fd_max = new_fd;
                        }

                        printf("selectserver: new connection from %s on socket %d\n",
								inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr*)&client_addr),
									client_ip, INET6_ADDRSTRLEN), new_fd);
                    }
                } else {
                    if ((num_bytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        if (num_bytes == 0) {
                            printf("selectserver: socket %d hung up\n", i);

                        } else {
                            perror("recv");
                        }

                        close(i);
                        FD_CLR(i, &master);
                    } else {
//                        // we got some data from a client
//                        for(j = 0; j <= fdmax; j++) {
//                            // send to everyone!
//                            if (FD_ISSET(j, &master)) {
//                                // except the listener and ourselves
//                              if (j != listener && j != i) {
//                                   if (send(j, buf, nbytes, 0) == -1) {
//                                        perror("send");
//                                    }
//                                }
//                            }
//                      }
                    }
                }
            } 
		}
    }
}

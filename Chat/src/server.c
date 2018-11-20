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

	uint32_t crc;
	uint16_t length;
	char * buf = buffer;

	length = (uint16_t) buf[2] << 8 | buf[3];
	crc = (uint32_t) buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];
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

	struct threadpool * pool = args;
	struct task job;

	uint8_t type;

	printf("## server_thread started\n");

	while (1) {
		read_fds = master;
        if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1) {
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

                        printf("server_thread: new connection from %s on socket %d\n",
								inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr*)&client_addr),
									client_ip, INET6_ADDRSTRLEN), new_fd);
                    }
                } else {
                    if ((num_bytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        if (num_bytes == 0) {
                            printf("server_thread: socket %d hung up\n", i);

                        } else {
                            perror("recv");
                        }

                        close(i);
                        FD_CLR(i, &master);
                    } else {
						type = buf[1];
						job.arg = buf;
						
						switch (type) {
							case 1:
								job.routineForTask = recv_sign_in;
								break;
							case 2:
								job.routineForTask = recv_sign_up;
								break;
							case 3:
								job.routineForTask = recv_member_list;
								break;
							case 4:
								job.routineForTask = recv_msg;
								break;
							case 5:
								job.routineForTask = recv_error;
								break;
							default:
								printf("server_thread: error type!\n");
								continue;
								break;
						}

						thpool_add_task(pool, job, 1);
						printf("Task added.\n");
                    }
                }
            } 
		}
    }
}

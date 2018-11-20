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

#include "client.h"


int sockfd;
struct addrinfo hints;
int yes = 1;

int
client_init (char * server_ip) {
	struct addrinfo *servlist, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((errno = getaddrinfo(server_ip, PORT, &hints, &servlist)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errno));
		return 1;
	}

	for(p = servlist; p != NULL; p = p->ai_next) {
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}

	freeaddrinfo(servlist);

	if(p == NULL) {
		errno = EPERM;
		return errno;
	}

	return 0;
}

void *
client_thread (void *args ) {
	char id;
	char msg[1024];
	char name_id[16];
	char cmd[10];

	while(1) {
		fflush(stdin);
		id = (char) getc(stdin);

		if (id == '@') {
			fgets(name_id, 16, stdin);
			fgets(msg, 1024, stdin);
		
		} else if(id == '/') {
			fgets(cmd, 10, stdin);
			
			if (strcmp(cmd, "quit") == 0) {
			
			} else if (strcmp(cmd, "info") == 0) {
			
			} else {
				printf("usage.... ");
			}
		
		} else {
			printf("usage....");
		}
		
	}
}

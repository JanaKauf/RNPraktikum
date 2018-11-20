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

int
send_all_data (char *buf, int *length) {
	int total = 0;
	int bytes_left = *length;
	int n;

	while (total < *length) {
		n = send(sockfd, buf+total, bytes_left, 0);
		if (n == -1)
			break;
		total += n;
		bytes_left -= n;
	}

	*length = total;

	return n==-1?-1:0;
}

int
client_connect_send (char * server_ip, char * buf) {
	struct addrinfo *servlist, *p;
	struct addrinfo hints;
	int yes = 1;
	int length = strlen(buf);

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

	if (send_all_data(buf, &length) == -1) {
		errno = EPERM;
		printf("Only send %d bytes\n", length);
		close(sockfd);
		return errno;
	
	}

	close(sockfd);	

	return 0;
}


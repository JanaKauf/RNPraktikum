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
#include "client.h"
#include "packet.h"
#include "color.h"

int sock_fd;
char *protocol;

void
Client_protocol(char *_protocol) {
	protocol = _protocol;
}

int
Client_connect (char * server_ip) {
	if (strcmp(protocol, "-sctp") == 0) {
		printf("CLIENT SCTP\n");
		struct sockaddr_in sin[1], serv;

		serv.sin_family = AF_INET;
		serv.sin_port = htons(6100);
		serv.sin_addr.s_addr = htonl(inet_addr(server_ip));

		sock_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP);
		if (sock_fd == -1) {
			return -1;
		}

		if (connect(sock_fd, (struct sockaddr *)&serv, sizeof(serv)) != 0) {
			close(sock_fd);
		}

	} else {
		printf("CLIENT TCP\n");
		printf("Connect %s\n", server_ip);
		struct addrinfo *servlist, *p;
		struct addrinfo hints;
	
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
	
		if((errno = getaddrinfo(server_ip, PORT, &hints, &servlist)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errno));
			return -1;
		}
	
		for(p = servlist; p != NULL; p = p->ai_next) {
			if((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
				perror("client: socket");
				continue;
			}
	
			if(connect(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
				close(sock_fd);
				perror("client: connect");
				continue;
			}
			break;
		}
	
		freeaddrinfo(servlist);
	
		if(p == NULL) {
			errno = EPERM;
			return -1;
		}
	}

	return sock_fd;
}

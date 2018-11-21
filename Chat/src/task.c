#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include "list.h"

#define PORT "6100"


//###################CONNECT_TASKS#######################
struct args_connect{
	char * ip;
	int * sock_fd;
};


void
connect_to_server (void* args) {
	struct args_connect connect_args = args;
	int sockfd;
	struct addrinfo *servlist, *p;
	struct addrinfo hints;
	int yes = 1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((errno = getaddrinfo(connect_args->ip, PORT, &hints, &servlist)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errno));
		return ;
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
		perror("connect: ");
		return ;
	}

	connect_args->sock_fd = &sockfd;

}

void
disconnect_from_server (void * socket) {
	int * sock_fd = socket;
	close(*sock_fd);

}

//######################SEND_TASKS###############################

struct args_send{
	int * sock_fd;
	void * buf;
	struct threadpool * send_pool;
	struct threadpool * connect_pool;

};

int
send_all_data (char *buf, int *length, int * socket) {
	int total = 0;
	int bytes_left = *length;
	int n;

	while (total < *length) {
		n = send(*socket, buf+total, bytes_left, 0);
		if (n == -1)
			break;
		total += n;
		bytes_left -= n;
	}

	*length = total;

	return n==-1?-1:0;
}

void
send_to_server (void * args) {
	struct args_send * send_args = args;
	int length = strlen(send_args->buf);

	if (send_all_data(send_args->buf, &length, send_args->sock_fd) == -1) {
		errno = EPERM;
		printf("Only send %d bytes\n", length);

		//send failed so we try to connect and send again
		struct task task_to_connect;
		struct args_connect * connect_args;
		struct sockaddr* addr;
		socklen_t* addr_length;
		char ip_addr[INET_ADDRSTRLEN];
		getpeername(*(send_args->sock_fd), addr, addr_length);
		connect_args->ip = inet_ntop(AF_INET, &(((struct sockaddr_in)addr)->sin_addr), ip_addr, INET_ADDRSTRLEN);
		connect_args->sock_fd = send_args->sock_fd;
		task_to_connect.arg = connect_args;
		task_to_connect.routine_for_task = connect_to_server;


		//task_to_send.arg = _args->;
		struct task task_to_send;
		task_to_send.routine_for_task = send_to_server;
		task_to_send.arg = send_args;

		thpool_add_task(send_args->connect_pool, task_to_connect, 1);
		thpool_add_task(send_args->send_pool, task_to_send, 1);
		close(send_args->sock_fd);
		perror("send_to_server: ");
		return ;
	
	}

	return ;
}


void
send_sign_in (void * buffer) {
}

void
send_quit (void * buffer) {
}

void
send_msg (void * buffer) {
}

void
send_member_list (void * buffer) {
}

void
send_error(void *buffer) {
}



//######################RECV_TASKS###############################
int
recv_sign_in (char * id, const uint32_t ip, const uint16_t port) {
	if (new_member(id, ip, port) != 0) {
		return -1;
	}

	printf("sign in: %s\n", id);	
}

int
recv_quit (char * id) {
	if (delete_member(id) == 0) {
		return 0;
	}
	return 1;
}

int
recv_msg (char * msg, const uint32_t ip) {
	struct member * messeger;

	if ((messeger = search_member_ip(ip)) == NULL) {
		return -1;
	}

	printf("@%s: ", messeger->id);

	for (char * i = msg; *i != '\0'; i = i++) {
		printf("%c", *i);
	}
	printf("\n");

	return 0;
}

void
recv_member_list (char * buffer) {

}

int
recv_error(char * error, const uint32_t ip) {
	struct member * messeger;

	if ((messeger = search_member_ip(ip)) != NULL) {
		printf("@%s: ", messeger->id);
	}

	printf("error: %s\n", error);

	return 0;
}

void
recv_from_client (void * socket) {
	uint8_t type;	
	uint16_t length;
	uint32_t crc;
	char * payload;

	int num_bytes;

	char * buf;
	int * new_fd = socket;

	struct sockaddr_in client_ip;
	socklen_t addr_size = sizeof(client_ip);
	
    if ((num_bytes = recv(*new_fd, buf, sizeof buf, 0)) <= 0) {
		if (num_bytes == 0) {
			printf("server_thread: socket %d hung up\n", new_fd);

		} else {
	        perror("recv:");
        }

        close(*new_fd);
	} else {

		type = buf[1];
		length = (uint16_t) buf[2] << 8 | buf[3];
		crc = (uint32_t) buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];

		payload = &buf[8];
		
		getpeername(*new_fd, (struct sockaddr *)&client_ip, &addr_size);
				
		switch (type) {
			case 1:
				recv_sign_in(payload, client_ip.sin_addr.s_addr, 6100);
				break;
			case 2:
				recv_quit(payload);
				close(*new_fd);
				break;
			case 3:
				recv_member_list(payload);
				break;
			case 4:
				recv_msg(payload, client_ip.sin_addr.s_addr);
				break;
			case 5:
				recv_error(payload, client_ip.sin_addr.s_addr);
				break;
			default:
				printf("server_thread: error type!\n");
				break;
		}
	}

}



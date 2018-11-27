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
#include <stdint.h>

#include "packet.h"
#include "list.h"
#include "list_thrsafe.h"
#include "task.h"
//#include "taskqueue.h"
#include "chat.h"
#include "thpool.h"
#include "packet.h"
#include "../libcrc/src/crc32.c"

#define PORT "6100"


//###################CONNECT_TASKS#######################
void
connect_to_server (char * ip, int * sockfd) {
	struct addrinfo *servlist, *p;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((errno = getaddrinfo(ip, PORT, &hints, &servlist)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errno));
		return ;
	}

	for(p = servlist; p != NULL; p = p->ai_next) {
		if((*sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if(connect(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(*sockfd);
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
	//TODO put sockfd in member list

}

void
disconnect_from_server (int * sockfd) {
	int * sock_fd = sockfd;
	if(close(*sock_fd) < 0) {
		perror("disconnect_from_server: ");
	}
}

//######################SEND_TASKS###############################


int
send_all_data (struct packet *packet, int *length, int *sockfd) {
	int total = 0;
	int bytes_left = *length;
	int n;

	while (total < *length) {
		n = send(*sockfd, packet+total, bytes_left, 0);
		if (n == -1) {
			break;
		}
		total += n;
		bytes_left -= n;
	}

	*length = total;

	return n==-1?-1:0;
}

void
send_to_server (struct packet* packet, int * sockfd) {
//	struct args_send *send_args = args;
	int length = sizeof(struct packet);

	if (send_all_data(packet, &length, sockfd) == -1) {
		struct args_send send_args;
		send_args.sock_fd = sockfd;
		send_args.buf = packet;
//		resend_packet(&send_args);


		errno = EPERM;
		printf("Only send %d bytes\n", length);

		close(*sockfd);
		perror("send_to_server: ");
		return ;
	
	}
	if (packet->typ == SIGN_IN) {
		close(*sockfd);
	}

	return ;
}

void resend_packet(void * arg) {
	//send failed so we try to connect and send again
	struct args_send* send_args = arg;

	struct member* p = List_get_list();

//	connect_to_server(List_get_ip_by_sockfd(*send_args->sock_fd))->ip);

//	if(send(*conn_args.sock_fd, send_args->buf, sizeof(*send_args->buf), 0) < 0) {
//		perror("resend_packet: ");
//	}

//	disconnect_from_server(&sock_fd);
}

void
send_sign_in (void * arg) {
	int i;
	int j;
//	char * ip = "141.22.88.84";
	char * ip = (char *) arg;
	printf("ip: %s, arg: %s\n", ip, (char *)arg);
	struct packet packet;
	struct member *p = List_get_list();
	int num_of_members = List_no_of_members();
	uint16_t bufsize = ((num_of_members * SIZE_OF_MEMBER_IN_BYTES) + 1);
	int sock_fd;

	connect_to_server(ip, &sock_fd);


	//header of member list
	packet.version = VERSION; //version
	packet.typ = SIGN_IN; //type
	packet.length = htons(bufsize);


	int ip_addr_offset = 1;
	int id_offset;

	//set content of member list
	uint8_t length_in_byte = (uint8_t)num_of_members;
	packet.payload[0] = length_in_byte;
	for(i = 0; i < num_of_members; i++) {
		//store in network byteorder
		packet.payload[0 + ip_addr_offset] = (p->ip & 0xFF000000) >> 24;
		packet.payload[1 + ip_addr_offset] = (p->ip & 0x00FF0000) >> 16;
		packet.payload[2 + ip_addr_offset] = (p->ip & 0x0000FF00) >> 8;
		packet.payload[3 + ip_addr_offset] = (p->ip & 0x000000FF);

		id_offset = 4 + ip_addr_offset;
		for(j = 0; j < ID_LENGTH; j++) {
			packet.payload[id_offset + j] = p->id[j];
		}

		ip_addr_offset += SIZE_OF_MEMBER_IN_BYTES;
		p = p->next;
	}
	packet.crc = htonl(crc_32(packet.payload, bufsize));

	send_to_server(&packet, &sock_fd);
	//TODO put sock_fd in memberlist?

}

void
send_quit (void * args) {
	struct args_send* send_args = args;
	int i;
	struct packet packet;
	//header of member list
	packet.version = VERSION; //version
	packet.typ = SIGN_OUT; //type
	packet.length = htons(ID_LENGTH); //length
	packet.crc = htonl(crc_32(ID_NAME, ID_LENGTH)); //TODO use define???
	strcpy(packet.payload, ID_NAME);

	struct member * p = NULL;
	int * sock_fd = NULL;
	struct in_addr i_ip;

//	TODO put sends in taskqueue? or send_quit gets called more often in other function?

	for (p = List_get_list(); p != NULL; p = p->next) {
		i_ip.s_addr = p->ip;
		sock_fd = List_get_sockfd_by_ip(p->ip);

		if (sock_fd == NULL) {
			connect_to_server(inet_ntoa(i_ip), sock_fd);
		}

		send_to_server(&packet, send_args->sock_fd);
	}

}

void
send_msg (void * buffer) {
	uint8_t* id = strtok(buffer, " \n\0");
	id = ++id; //remove @ from id
	uint8_t * msg = (uint8_t *)strtok(NULL, " \n\0");
	uint16_t bufsize = sizeof(msg);
	struct member messeger = List_search_member_id(id);
	int * sock_fd = List_get_sockfd_by_id(id);
	struct packet packet;

	printf("msg: %s\n", (char *)buffer);

	struct in_addr i_ip;
	i_ip.s_addr = messeger.ip;

	if (sock_fd == NULL) {
		connect_to_server(inet_ntoa(i_ip), sock_fd);
	}

	//header of member list
	packet.version = VERSION; //version
	packet.typ = MESSAGE; //type
	packet.length = htons(bufsize);
	packet.crc = htonl(crc_32(msg, bufsize));

	strcpy(packet.payload, msg);

	send_to_server(&packet, sock_fd);
}

void
send_member_list (void * buffer) {
	struct member *p = List_get_list();
	int num_of_members = List_no_of_members();
	uint16_t bufsize = (num_of_members * SIZE_OF_MEMBER_IN_BYTES) + 1;
	struct packet packet;
	//header of member list
	packet.version = VERSION; //version
	packet.typ = MEMBER_LIST; //type
	packet.length = htons(bufsize); //length

	int i;
	int j;

	int ip_addr_offset = 1;
	int id_offset;

	//set content of member list
	uint8_t length_in_byte = (uint8_t)num_of_members;
	packet.payload[0] = length_in_byte;
	for(i = 0; i < num_of_members; i++) {
		//store in network byteorder
		packet.payload[0 + ip_addr_offset] = (p->ip & 0xFF000000) >> 24;
		packet.payload[1 + ip_addr_offset] = (p->ip & 0x00FF0000) >> 16;
		packet.payload[2 + ip_addr_offset] = (p->ip & 0x0000FF00) >> 8;
		packet.payload[3 + ip_addr_offset] = (p->ip & 0x000000FF);

		id_offset = 4 + ip_addr_offset;
		for(j = 0; j < ID_LENGTH; j++) {
			packet.payload[id_offset + j] = p->id[j];
		}

		ip_addr_offset += SIZE_OF_MEMBER_IN_BYTES;
		p = p->next;
	}
	packet.crc = htonl(crc_32(packet.payload, bufsize));

	send_to_server(&packet, (int *)buffer);
}

void
send_error(void *buffer) {
	uint8_t * payload = (uint8_t *)"Error";
	uint16_t string_length = 5;
	struct packet packet;

	//header of member list
	packet.version = VERSION; //version
	packet.typ = ERROR; //type
	packet.length = htons(string_length); //length
	packet.crc = crc_32(payload, string_length);
	strcpy(packet.payload, payload);

	send_to_server(&packet, buffer);
}



//######################RECV_TASKS###############################
int
recv_sign_in (uint8_t * buffer,
		const uint32_t ip_addr, int * sockfd) {

	uint8_t no_member = buffer[0];
	uint32_t ip;
	uint8_t *id;

	struct threadpool * send_pool;
	send_pool = Chat_get_sendpool();

	struct in_addr i_ip;

	struct task_t job;
	job.routine_for_task = send_member_list;
	job.arg = sockfd;
	Thpool_add_task(send_pool, job);

	int offset = 0;

	for (int i = 0; i < no_member; i++) {
		ip = (uint32_t) buffer[1 + offset] << 24
					| buffer[2 + offset] << 16
					| buffer[3 + offset] << 8
					| buffer[4 + offset];

		id = &buffer[5 + offset];	
		
		i_ip.s_addr = ip;

		if (Thrsafe_new_member(id, ip) != 0) {
			printf("task: recv_member_list fail to add id %s\n", id);
		
		}

		if (i == 0) {
			printf("sign in: %s sock_fd %d\n", id, *sockfd);	
		}

		offset += SIZE_OF_MEMBER_IN_BYTES;
	}

	return 0;
}

int
recv_quit (uint8_t *id) {

	struct member to_delete = List_search_member_id(id);

	disconnect_from_server(to_delete.sock_fd);

	if (Thrsafe_delete_member_id(id) != 0) {
		return -1;
	}

	return 0;
}

int
recv_msg (uint8_t *msg, const uint32_t ip) {
	struct member messeger;

	messeger = List_search_member_ip(ip);
	
	printf("#%s: %s\n", messeger.id, msg);

	return 0;
}

int
recv_member_list (uint8_t *buffer) {
	uint8_t no_member = buffer[0];
	uint32_t ip;
	char *id;

	int offset = 0;

	for (int i = 0; i < no_member; i++) {
		ip = (uint32_t) buffer[1 + offset] << 24
					| buffer[2 + offset] << 16
					| buffer[3 + offset] << 8
					| buffer[4 + offset];

		id = &buffer[5 + offset];	

		if (Thrsafe_new_member(id, ip) != 0) {
			printf("task: recv_member_list fail to add id %s\n", id);
		
		}

		printf("id: %s\n", id);
		printf("ip: %u\n", ip);
		offset += SIZE_OF_MEMBER_IN_BYTES;
	}

	return 0;
}

int
recv_error(uint8_t *error, const uint32_t ip) {
	struct member messeger;

	messeger = List_search_member_ip(ip);
	printf("#%s: %s\n", messeger.id, error);

	return 0;
}

void
recv_from_client (void *sockfd) {
	packet_t pck;

	uint8_t type;	
	uint16_t length;
	uint32_t crc;
	uint8_t * payload;

	int num_bytes;

//	unsigned char buf[1024];
	int *new_fd = sockfd;

	struct sockaddr_in client_ip;
	socklen_t addr_size = sizeof(client_ip);
	
    if ((num_bytes = recv(*new_fd, (struct packet *)&pck, sizeof (struct packet), 0)) <= 0) {
		if (num_bytes == 0) {
			printf("server_thread: sockfd %d hung up\n", *new_fd);

		} else {
	        perror("recv:");
        }

        close(*new_fd);
	} else {
		type = pck.typ;

		printf("type: %u\n", type);

		length = ntohs(pck.length);
		printf("length: %u\n", length);

		crc = ntohl(pck.crc);
		printf("crc: %u\n", crc);

//		num_bytes -= 8;

//		payload = malloc(num_bytes);	

//		payload = pck.payload;	

		getpeername(*new_fd,
				(struct sockaddr *)&client_ip,
				&addr_size);
				
		switch (type) {
			case SIGN_IN:
				recv_sign_in(pck.payload, client_ip.sin_addr.s_addr, new_fd);
				break;
			case SIGN_OUT:
				close(*new_fd);
				recv_quit(pck.payload);
				break;
			case MEMBER_LIST:
				recv_member_list(pck.payload);
				break;
			case MESSAGE:
				recv_msg(pck.payload, client_ip.sin_addr.s_addr);
				break;
			case ERROR:
				recv_error(pck.payload, client_ip.sin_addr.s_addr);
				break;
			default:
				printf("server_thread: error type!\n");
				break;
		}
	}

}



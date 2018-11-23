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
#include "taskqueue.h"
#include "chat.h"
#include "thpool.h"

#define PORT "6100"


//###################CONNECT_TASKS#######################
void
connect_to_server (void *args) {
	struct args_connect *connect_args = (struct args_connect *)args;
	int sock_fd;
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
		perror("connect: ");
		return ;
	}
	//TODO put sockfd in member list
	connect_args->sock_fd = &sock_fd;

}

void
disconnect_from_server (void * sockfd) {
	int * sock_fd = sockfd;
	close(*sock_fd);

}

//######################SEND_TASKS###############################


int
send_all_data (struct packet *packet, int *length, int *sockfd) {
	int total = 0;
	int bytes_left = *length;
	int n;

	while (total < *length) {
		n = send(*sockfd, packet+total, bytes_left, 0);
		if (n == -1)
			break;
		total += n;
		bytes_left -= n;
	}

	*length = total;

	return n==-1?-1:0;
}

void
send_to_server (struct packet* packet, int * sockfd) {
//	struct args_send *send_args = args;
	int length = sizeof(packet);

	if (send_all_data(packet, &length, sock_fd) == -1) {
		errno = EPERM;
		printf("Only send %d bytes\n", length);

		//send failed so we try to connect and send again
//		struct task task_to_connect;
//		struct args_connect *connect_args;
		struct sockaddr *addr;
		socklen_t *addr_length;
		char ip_addr[INET_ADDRSTRLEN];

//		getpeername(*sock_fd, addr, addr_length);

//		connect_args->ip = inet_ntop(AF_INET, &(((struct sockaddr_in *)addr)->sin_addr), ip_addr, INET_ADDRSTRLEN);
//		connect_args->sock_fd = sock_fd;

//		task_to_connect.arg = connect_args;
//		task_to_connect.routine_for_task = connect_to_server;
//		Thpool_add_task(Chat_get_sendpool(), task_to_connect, 1);
//
//		//task_to_send.arg = _args->;
//		struct task task_to_send;
//		task_to_send.routine_for_task = send_to_server;
//		task_to_send.arg = send_args;
//
//		Thpool_add_task(Chat_get_sendpool(), task_to_send, 1);
		if (packet->typ == SIGN_IN) {
			close(*sock_fd);
		}
//		close(*sock_fd);
		perror("send_to_server: ");
		return ;
	
	}

	return ;
}


void
send_sign_in (void * arg) {
	int i;
	int j;
	char * ip = arg;
	struct packet packet;
	struct member *p = list;
	int num_of_members = List_no_of_members();
	uint16_t bufsize = ((num_of_members * SIZE_OF_MEMBER_IN_BYTES) + SIZE_OF_HEADER_IN_BYTES);
	int sock_fd;

	struct args_connect con_args;
	con_args.sock_fd = &sock_fd;
	con_args.ip = ip;

	connect_to_server(con_args);

	uint8_t payload[bufsize];
	//unsigned char payload[bufsize];

	//header of member list
	packet.version = VERSION; //version
	packet.typ = SIGN_IN; //type
	packet.length = htons(bufsize);
	packet.payload = payload;

	//TODO get CRC
	//uint32_t crc
	//sign_in_buf[4] = crc & 0xFF000000 >> 24;
	//sign_in_buf[5] = crc & 0x00FF0000 >> 16;
	//sign_in_buf[6] = crc & 0x0000FF00 >> 8;
	//sign_in_buf[7] = crc & 0x000000FF;



	int ip_addr_offset;
	int id_offset;
	uint32_t ip_to_send;
	//set content of member list
	for(i = 0; i < num_of_members; i++) {
		ip_addr_offset = i * SIZE_OF_MEMBER_IN_BYTES;
		//store in network byteorder
		payload[0 + ip_addr_offset] = (p->ip & 0xFF000000) >> 24;
		payload[1 + ip_addr_offset] = (p->ip & 0x00FF0000) >> 16;
		payload[2 + ip_addr_offset] = (p->ip & 0x0000FF00) >> 8;
		payload[3 + ip_addr_offset] = (p->ip & 0x000000FF);

		id_offset = 4 + ip_addr_offset;
		for(j = 0; j < ID_LENGTH; j++) {
			payload[id_offset + j] = p->id[j];
		}

		p = p->next;
	}
	packet.payload = payload;
	send_to_server(&packet, &sock_fd);

}

void
send_quit (void * buffer) {
	struct member *p = list;
	int num_of_members = List_no_of_members();
	uint16_t bufsize = ID_LENGTH;
	unsigned char payload[bufsize];
	struct packet packet;
	//header of member list
	packet.version = VERSION; //version
	packet.typ = SIGN_OUT; //type
	packet.length = htons(bufsize); //length
	//TODO get CRC
	//uint32_t crc
	//sign_in_buf[4] = crc & 0xFF000000 >> 24;
	//sign_in_buf[5] = crc & 0x00FF0000 >> 16;
	//sign_in_buf[6] = crc & 0x0000FF00 >> 8;
	//sign_in_buf[7] = crc & 0x000000FF;

	int i;
	for(i = 0; i < ID_LENGTH; i++) {
		packet.payload[i] = list->id[i];
	}




}

void
send_msg (void * buffer) {
	struct member *p = list;
	char* id = strtok(buffer, " \n\0");
	id = ++id; //remove @ from id
	char * msg = strtok(NULL, " \n\0");

	int sock_fd = List_get_sockfd_by_id();
	int num_of_members = List_no_of_members();
	struct packet packet;
	//header of member list
	packet.version = VERSION; //version
	packet.typ = MESSAGE; //type
	packet.length = htonl(sizeof(msg)); //size_t's length can vary :/
	packet.payload = msg;
//	packet.crc

	send_to_server(&packet, &sock_fd);
}

void
send_member_list (void * buffer) {
	struct member *p = list;
	int num_of_members = List_no_of_members();
	uint16_t bufsize = num_of_members * SIZE_OF_MEMBER_IN_BYTES;
	uint8_t payload[bufsize];
	struct packet packet;
	//header of member list
	packet.version = VERSION; //version
	packet.typ = MEMBER_LIST; //type
	packet.length = htons(bufsize); //length
	//TODO get CRC
	//uint32_t crc
	//sign_in_buf[4] = crc & 0xFF000000 >> 24;
	//sign_in_buf[5] = crc & 0x00FF0000 >> 16;
	//sign_in_buf[6] = crc & 0x0000FF00 >> 8;
	//sign_in_buf[7] = crc & 0x000000FF;

	int i;
	int j;
	int member_offset;
	int ip_addr_offset = sizeof(uint32_t);
	for(i = 0; i < num_of_members; i++) {
		member_offset = i * SIZE_OF_MEMBER_IN_BYTES;
		payload[member_offset + 0] = (p->ip & 0xFF000000) >> 24;
		payload[member_offset + 1] = (p->ip & 0x00FF0000) >> 16;
		payload[member_offset + 2] = (p->ip & 0x0000FF00) >> 8;
		payload[member_offset + 3] = (p->ip & 0x000000FF);


		for(j = ip_addr_offset + member_offset; j < member_offset + SIZE_OF_HEADER_IN_BYTES; j++) {
			payload[j] = list->id[j];
		}
		p = p->next;
	}
	packet.payload = payload;
	send_to_server(packet, buffer);
}

void
send_error(void *buffer) {
	struct member *p = list;
	int num_of_members = List_no_of_members();
	uint16_t bufsize = 8;
	uint8_t payload[bufsize] = "Error";
	struct packet packet;
	//header of member list
	packet.version = VERSION; //version
	packet.typ = ERROR; //type
	packet.length = htons(bufsize); //length
	packet.payload = payload;

	send_to_server(packet, buffer);
}



//######################RECV_TASKS###############################
int
recv_sign_in (unsigned char * buffer,
		const uint32_t ip_addr, int length, int * sockfd) {

	uint32_t ip;
	char *id;

//	int *sock_fd;

//	struct args_connect c_args;
//	struct in_addr i_ip;

	int counter = 0;

	for (int i = 0; i < length; i++) {
		ip = (uint32_t) buffer[0+counter] << 24
					| buffer[1+counter] << 16
					| buffer[2+counter] << 8
					| buffer[3+counter];

		id = &buffer[4+counter];	
		
//		i_ip.s_addr = ip;
//		c_args.ip = inet_ntoa(i_ip);
//		c_args.sock_fd = sock_fd;

//		connect_to_server(&c_args);

		if (Thrsafe_new_member(id, ip, sockfd) != 0) {
			printf("task: recv_member_list fail to add id %s\n", id);
		
		}

		if (i == 0) {
			printf("sign in: %s\n", id);	
		}

		counter += SIZE_OF_MEMBER_IN_BYTES;
	}

}

int
recv_quit (unsigned char *id) {
	if (Thrsafe_delete_member_id(id) != 0) {
		return -1;
	}

	return 0;
}

int
recv_msg (unsigned char *msg, const uint32_t ip) {
	struct member messeger;

	messeger = List_search_member_ip(ip);
	
	printf("@%s: ", messeger.id);

	for (char * i = msg; *i != '\0'; i = i++) {
		printf("%c", *i);
	}
	printf("\n");

	return 0;
}

int
recv_member_list (unsigned char *buffer, uint16_t length, int *sockfd) {
	uint32_t ip;
	char *id;

//	int *sock_fd;

//	struct args_connect c_args;
//	struct in_addr i_ip;

	int counter = 0;

	for (int i = 0; i < length; i++) {
		ip = (uint32_t) buffer[0+counter] << 24
					| buffer[1+counter] << 16
					| buffer[2+counter] << 8
					| buffer[3+counter];

		id = &buffer[4+counter];	

//		i_ip.s_addr = ip;
//		c_args.ip = inet_ntoa(i_ip);
//		c_args.sock_fd = sock_fd;

//		connect_to_server(&c_args);

		if (Thrsafe_new_member(id, ip, sockfd) != 0) {
			printf("task: recv_member_list fail to add id %s\n", id);
		
		}
		counter += SIZE_OF_MEMBER_IN_BYTES;
	}

}

int
recv_error(unsigned char *error, const uint32_t ip) {
	struct member messeger;

	messeger = List_search_member_ip(ip);
	printf("@%s: ", messeger.id);

	printf("error: %s\n", error);

	return 0;
}

void
recv_from_client (void *sockfd) {
	uint8_t type;	
	uint16_t length;
	uint32_t crc;
	unsigned char *payload;

	int num_bytes;

	unsigned char * buf;
	int *new_fd = sockfd;

	struct sockaddr_in client_ip;
	socklen_t addr_size = sizeof(client_ip);
	
    if ((num_bytes = recv(*new_fd, buf, sizeof buf, 0)) <= 0) {
		if (num_bytes == 0) {
			printf("server_thread: sockfd %d hung up\n", new_fd);

		} else {
	        perror("recv:");
        }

        close(*new_fd);
	} else {

		type = buf[1];

		length = (uint16_t) buf[2] << 8
							| buf[3];

		crc = (uint32_t) buf[4] << 24
						| buf[5] << 16
						| buf[6] << 8
						| buf[7];

		payload = &buf[8];
		
		getpeername(*new_fd,
				(struct sockaddr *)&client_ip,
				&addr_size);
				
		switch (type) {
			case SIGN_IN:
				recv_sign_in(payload, client_ip.sin_addr.s_addr, length, new_fd);
				break;
			case SIGN_OUT:
				close(*new_fd);
				recv_quit(payload);
				break;
			case MEMBER_LIST:
				recv_member_list(payload, length, new_fd);
				break;
			case MESSAGE:
				recv_msg(payload, client_ip.sin_addr.s_addr);
				break;
			case ERROR:
				recv_error(payload, client_ip.sin_addr.s_addr);
				break;
			default:
				printf("server_thread: error type!\n");
				break;
		}
	}

}



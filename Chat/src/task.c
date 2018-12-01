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
//#include "list_thrsafe.h"
#include "task.h"
#include "client.h"
#include "chat.h"
#include "thpool.h"
#include "packet.h"
#include "color.h"
#include "../libcrc/src/crc32.c"

#define PORT "6100"

pthread_mutex_t mutex;
bool sign;

int
Tasks_start() {
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		perror("Tasks_start: send_mutex");
		return -1;
	}

	sign = false;

	return 0;
}

int
Tasks_clean (void) {

	if (List_delete()) {
		perror("Tasks_start: ");
		return -1;
	}
	pthread_mutex_destroy(&mutex);

	return 0;

}


//######################SEND_TASKS###############################

int
send_all_data (struct packet *packet, int *length, int sockfd) {
	int total = 0;
	int bytes_left = *length;
	int n;

	while (total < *length) {
		n = send(sockfd, packet+total, bytes_left, 0);
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
send_to_server (struct packet* packet, int sockfd) {
	int length = sizeof(struct packet);

	if (send_all_data(packet, &length, sockfd) == -1) {
//		struct args_send send_args;
//		send_args.sock_fd = sockfd;
//		send_args.buf = packet;
//		resend_packet(&send_args);


		errno = EPERM;
		printf("Only send %d bytes\n", length);

		perror("send_to_server: ");
		return ;
	
	}

	return ;
}


void resend_packet(void * arg) {
//	send failed so we try to connect and send again
//	struct args_send* send_args = arg;
//
//	struct member* p = List_get_list();
//
//	connect_to_server(List_get_ip_by_sockfd(*send_args->sock_fd))->ip);
//
//	if(send(*conn_args.sock_fd, send_args->buf, sizeof(*send_args->buf), 0) < 0) {
//		perror("resend_packet: ");
//	}
//
//	disconnect_from_server(&sock_fd);
}


void
send_sign_in (void * arg) {
	printf(BLU "#\t#\t#\t#\tsend_sign_in()\t#\t#\t#\t#\n" RESET);
	int i;
	int j;
	char * ip = (char *) arg;
	struct packet packet;
	pthread_mutex_lock(&mutex);
	struct member *p = List_get_list();
	uint16_t bufsize = ((List_no_of_members() * SIZE_OF_MEMBER_IN_BYTES) + 1);

	int sock_fd = Client_connect(ip);

	if (sock_fd == -1) {
		perror(RED "Client_connect: send_sign_in" RESET);
		return ;
	}


	//header of member list
	packet.version = VERSION; //version
	packet.typ = SIGN_IN; //type
	packet.length = htons(bufsize);
	packet.payload[0] = (uint8_t)List_no_of_members();

	int ip_addr_offset = 1;
	int id_offset;

	//set content of member list
	for(i = 0; i < List_no_of_members(); i++) {
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

	send_to_server(&packet, sock_fd);

	sign = true;

	if (close(sock_fd) != 0)
		perror(RED "Close: send_sign_in" RESET);
	pthread_mutex_unlock(&mutex);


}

void
send_quit (void * args) {
	printf(BLU "#\t#\t#\t#\tsend_quit()\t#\t#\t#\t#\n" RESET);
	struct packet packet;

	struct member *me = List_get_list();

	//header of member list
	packet.version = VERSION; //version
	packet.typ = SIGN_OUT; //type
	packet.length = htons(ID_LENGTH); //length
	packet.crc = htonl(crc_32(me->id, ID_LENGTH));
	strcpy((char*)packet.payload, (char*)me->id);

	struct member * p = NULL;
	struct in_addr i_ip;

	int sock_fd;

	pthread_mutex_lock(&mutex);
	for (p = List_get_list()->next; p != NULL; p = p->next) {
		i_ip.s_addr = p->ip;

		sock_fd = Client_connect(inet_ntoa(i_ip));
		if (sock_fd == -1) {
			perror(RED "Client_connect: send_quit" RESET);
			continue ;
		}

		send_to_server(&packet, sock_fd);

		if (close(sock_fd) != 0)
			perror(RED "Close: send_quit" RESET);
	}
	pthread_mutex_unlock(&mutex);

}

void
send_msg (void * buffer) {
	printf(BLU "#\t#\t#\t#\tsend_msg()\t#\t#\t#\t#\n" RESET);
	uint8_t* id = (uint8_t*)strtok(buffer, " \n\0");
	id++; //remove @ from id
	uint8_t * msg = (uint8_t *)strtok(NULL, "\n\0");
	uint16_t bufsize = sizeof(msg);
	pthread_mutex_lock(&mutex);
	struct member messeger = List_search_member_id(id);
	int sock_fd;
	struct packet packet;

	printf("msg: %s\n", msg);

	if (messeger.ip == 0) {
		perror(RED "send_msg ID not found" RESET);
		return ;
	}

	struct in_addr i_ip;
	i_ip.s_addr = messeger.ip;

	sock_fd = Client_connect(inet_ntoa(i_ip));

	if (sock_fd == -1) {
		perror(RED "Client_connect: send_msg" RESET);
		return ;
	}

	//header of member list
	packet.version = VERSION; //version
	packet.typ = MESSAGE; //type
	packet.length = htons(bufsize);
	packet.crc = htonl(crc_32(msg, bufsize));

	strcpy((char*)packet.payload, (char*)msg);

	send_to_server(&packet, sock_fd);

	if (close(sock_fd) != 0)
		perror(RED "Close: send_msg" RESET);
	pthread_mutex_unlock(&mutex);
}

void
send_member_list (void * arg) {
	printf(BLU "#\t#\t#\t#\tsend_member_list()\t#\t#\t#\t#\n" RESET);
	pthread_mutex_lock(&mutex);
	struct member *p = List_get_list();
	uint16_t bufsize = (List_no_of_members() * SIZE_OF_MEMBER_IN_BYTES) + 1;
	struct packet packet;
	uint8_t * id = (uint8_t *)arg;

	struct member messenger = List_search_member_id(id);
	int sock_fd;
	struct in_addr i_ip;
	i_ip.s_addr = messenger.ip;

	sock_fd = Client_connect(inet_ntoa(i_ip));

	if (sock_fd == -1) {
		perror(RED "Client_connect: send_member_list" RESET);
		return ;
	}

	//header of member list
	packet.version = VERSION; //version
	packet.typ = MEMBER_LIST; //type
	packet.length = htons(bufsize); //length
	packet.payload[0] = (uint8_t)List_no_of_members();

	int ip_addr_offset = 1;
	int id_offset;

	int i;
	int j;
	//set content of member list
	for(i = 0; i < List_no_of_members(); i++) {
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

	send_to_server(&packet, sock_fd);
	if (close(sock_fd) != 0)
		perror(RED "Close: send_member_list" RESET);
	pthread_mutex_unlock(&mutex);
}

void
send_error(void *buffer) {
	//TODO connect send and disconnect
	printf(BLU "#\t#\t#\t#\tsend_error()\t#\t#\t#\t#\n" RESET);
	uint8_t* payload = buffer;
	uint16_t string_length = 5;
	struct packet packet;
	int sock_fd;

	//header of member list
	packet.version = VERSION; //version
	packet.typ = ERROR; //type
	packet.length = htons(string_length); //length
	packet.crc = crc_32(payload, string_length);
	strcpy((char*)packet.payload, (char*)payload);

//	Client_connect();
//	if (sock_fd == -1) {
//		return ;
//	}
//	send_to_server(&packet, buffer);
//	Client_disconnect();
//	if (close(sock_fd) != 0)
//		perror("Close: ");
}


void
send_member_list_to_my_members (void * args) {
	uint8_t * payload = (uint8_t *)args;
	struct packet packet;
	struct member *p;
	struct in_addr i_ip;
	int sock_fd;

	packet.version = VERSION; //version
	packet.typ = MEMBER_LIST; //type
	packet.length = htons(sizeof(payload)); //length
	packet.crc = htonl(crc_32(packet.payload, sizeof(payload)));

	int i;
	for (i = 0; i <= sizeof(payload); i++) {
		packet.payload[i] = payload[i];
	}

	pthread_mutex_lock(&mutex);
	for (p = List_get_list()->next; p != NULL; p = p->next) {
		i_ip.s_addr = p->ip;

		sock_fd = Client_connect(inet_ntoa(i_ip));
		if (sock_fd == -1) {
			perror(RED "Client_connect: send_quit" RESET);
			continue ;
		}

		send_to_server(&packet, sock_fd);

		if (close(sock_fd) != 0)
			perror(RED "Close: send_quit" RESET);
	}
	pthread_mutex_unlock(&mutex);

}



//######################RECV_TASKS###############################
int
recv_sign_in (uint8_t * buffer,
		const uint32_t ip_addr, int sockfd) {
	printf(BLU "#\t#\t#\t#\trecv_sign_in()\t#\t#\t#\t#\n" RESET);

	uint8_t no_member = buffer[0];
	uint32_t ip;
	uint8_t *id;

	struct threadpool * send_pool;
	send_pool = Chat_get_sendpool();

	//TODO i_ip unused?
	struct in_addr i_ip;

	struct task_t job;

	int offset = 0;

	int i;
	for (i = 0; i < no_member; i++) {
		ip = (uint32_t) buffer[1 + offset] << 24
					| buffer[2 + offset] << 16
					| buffer[3 + offset] << 8
					| buffer[4 + offset];

		id = &buffer[5 + offset];
		
		i_ip.s_addr = ip;

		pthread_mutex_lock(&mutex);
		if (List_new_member(id, ip) != 0) {
			printf(RED "recv_sign_in: id double %s or ip double %u\n" RESET, id, ip);
		
		}
		pthread_mutex_unlock(&mutex);

		if (i == 0) {
			printf("sign in: %s sock_fd %d\n", id, sockfd);	

			job.routine_for_task = send_member_list;
			job.arg = malloc(sizeof(id));
			strcpy((char*)job.arg, (char*)id);
			job.mallfree = true;
			Thpool_add_task(send_pool, job);

			if (List_no_of_members() > 1) {
				job.routine_for_task = send_member_list_to_my_members;
				job.arg = malloc(sizeof(buffer));
				strcpy((char*)job.arg, (char*)buffer);
				job.mallfree = true;
				Thpool_add_task(send_pool, job);
			}
		}

		offset += SIZE_OF_MEMBER_IN_BYTES;
	}

	return 0;
}

int
recv_quit (uint8_t *id) {
	printf(BLU "#\t#\t#\t#\trecv_quit()\t#\t#\t#\t#\n" RESET);

	printf("@%s: quit\n", id);

	pthread_mutex_lock(&mutex);
	if (List_delete_member(id) != 0) {
		perror(RED "recv_quit: id not found" RESET);
		return -1;
	}
	pthread_mutex_unlock(&mutex);

	return 0;
}

int
recv_msg (uint8_t *msg, const uint32_t ip) {
	printf(BLU "#\t#\t#\t#\trecv_msg()\t#\t#\t#\t#\n" RESET);
	struct member messeger;

	pthread_mutex_lock(&mutex);
	messeger = List_search_member_ip(ip);
	pthread_mutex_unlock(&mutex);
	
	printf(YEL "#%s: %s\n" RESET, messeger.id, msg);

	return 0;
}

int
recv_member_list (uint8_t *buffer) {
	printf(BLU "#\t#\t#\t#\trecv_member_list()\t#\t#\t#\t#\n" RESET);
	uint8_t no_member = buffer[0];
	uint32_t ip;
	uint8_t *id;

	struct threadpool * send_pool;
	send_pool = Chat_get_sendpool();

	struct task_t job;

	int offset = 0;

	int i;
	for (i = 0; i < no_member; i++) {
		ip = (uint32_t) buffer[1 + offset] << 24
					| buffer[2 + offset] << 16
					| buffer[3 + offset] << 8
					| buffer[4 + offset];

		id = &buffer[5 + offset];	

		pthread_mutex_lock(&mutex);
		if (List_new_member(id, ip) != 0) {
			printf(RED "recv_member_list: id double %s or ip double %u\n" RESET, id, ip);
		
		}
		if (i == 0) {

			if (List_no_of_members() > 1 && sign) {
				job.routine_for_task = send_member_list_to_my_members;
				job.arg = malloc(sizeof(buffer));
				strcpy((char*)job.arg, (char*)buffer);
				job.mallfree = true;
				Thpool_add_task(send_pool, job);
			}
		}

		pthread_mutex_unlock(&mutex);

		printf(CYN "recv_member_list id: %s\n" RESET, id);
		printf(CYN "recv_member_list ip: %u\n\n" RESET, ip);
		offset += SIZE_OF_MEMBER_IN_BYTES;
	}

	sign = false;

	return 0;
}

int
recv_error(uint8_t *error, const uint32_t ip) {
	printf(BLU "#\t#\t#\t#\trecv_error()\t#\t#\t#\t#\n" RESET);
	struct member messeger;

	pthread_mutex_lock(&mutex);
	messeger = List_search_member_ip(ip);
	pthread_mutex_unlock(&mutex);
	printf(YEL "#%s: %s\n" RESET, messeger.id, error);

	return 0;
}

void
recv_from_client (void *sockfd) {
	packet_t pck;
	uint32_t crc;
	int num_bytes;

	int new_fd = *(int *)(sockfd);

	struct sockaddr_in client_ip;
	socklen_t addr_size = sizeof(client_ip);
	
    if ((num_bytes = recv(new_fd, (struct packet *)&pck, sizeof (struct packet), 0)) <= 0) {
		if (num_bytes == 0) {
			printf(MAG "recv_from_client: sockfd %d not there\n" RESET, new_fd);

		} else {
	        perror(RED "recv_from_client: recv" RESET);
        }

		if (close(new_fd)) 
		   perror(RED "Close: recv_from_client" RESET);	
	} else {

		printf("type: %u\n", pck.typ);

		printf("length: %u\n", ntohs(pck.length));

		crc = ntohl(pck.crc);
		printf("crc: %u\n", crc);
		if(!crc_is_equal((uint8_t *)&pck.payload, pck.length, crc)) {
			struct task_t job_error_crc;
			uint8_t* error_type = malloc(sizeof(uint8_t));
			*error_type = htons(ERROR_WRONG_CRC);

			job_error_crc.arg = error_type;
			job_error_crc.mallfree = true;
			job_error_crc.routine_for_task = send_error;
			Thpool_add_task(send_pool, job_error_crc);
		}

		getpeername(new_fd,
				(struct sockaddr *)&client_ip,
				&addr_size);
				
		switch (pck.typ) {
			case SIGN_IN:
				recv_sign_in(pck.payload, client_ip.sin_addr.s_addr, new_fd);
				break;
			case SIGN_OUT:
				recv_quit(pck.payload);
				if (close(new_fd) != 0)
					perror(RED "Close: recv_from_client" RESET);	
				break;
			case MEMBER_LIST:
				recv_member_list(pck.payload);
				if (close(new_fd) != 0)
					perror(RED "Close: recv_from_client" RESET);	
				break;
			case MESSAGE:
				recv_msg(pck.payload, client_ip.sin_addr.s_addr);
				if (close(new_fd) != 0)
					perror(RED "Close: recv_from_client" RESET);	
				break;
			case ERROR:
				recv_error(pck.payload, client_ip.sin_addr.s_addr);
				if (close(new_fd) != 0)
					perror(RED "Close: recv_from_client" RESET);	
				break;
			default:
				printf(RED "server_thread: error type\n" RESET);
				break;
		}
	}

}


//TODO put function in a different file???
/**
 * return 1 if crcToCheck and crc computed with strToCheck are equal else
 * return 0
 */
int crc_is_equal(uint8_t* strToCheck, uint16_t strLength, uint32_t crcToCheck) {
	return crc_32(strToCheck, strLength) == crcToCheck;
}



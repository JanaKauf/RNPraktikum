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

/**
 * Starts the mutex to protect List
 *
 * @return -1 on failure, 0 on success
 */

int
Tasks_start(void) {
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		perror("Tasks_start: mutex");
		return -1;
	}

	sign = false;

	return 0;
}

/**
 * Destroy list and mutex
 *
 * @return -1 on failure, 0 on success
 */

int
Tasks_clean (void) {

	if (List_delete()) {
		perror("Tasks_start: List_delete");
		return -1;
	}

	if (pthread_mutex_destroy(&mutex) != 0) {
		perror("Tasks_clean: mutex");
		return -1;
	}

	return 0;

}


//######################SEND_TASKS###############################

/**
 * Sends packet packet on socket sockfd with the length of length,
 * makes sure that the whole packet is send
 * @param packet the packet to be send
 * @param length the length of the packet
 * @param sockfd the socket descriptor to send the packet on
 *
 * @return -1 on failure, 0 on success
 */

int
send_all_data (struct packet *packet, int *length, int sockfd) {
	int total = 0;
	int bytes_left = *length;
	int n;

	while (total < *length) {
		n = send(sockfd, packet+total, bytes_left, 0);
		if (n == -1) {
			perror("send_all_data: ");
			break;
		}
		total += n;
		bytes_left -= n;
	}

	*length = total;

	return n == -1 ? -1 : 0;
}

/*
 * sends a packet packet on sockfd sockfd
 * @param packet the packet to be send
 * @param sockfd the socket descriptor to send the packet on
 *
 * @return -1 on failure, 0 on success
 */
int
send_to_server (struct packet* packet, int sockfd) {
	int length = sizeof(struct packet);

	if (send_all_data(packet, &length, sockfd) == -1) {
		errno = EPERM;
		printf("Only send %d bytes\n", length);

		perror("send_to_server: ");
		return -1;
	
	}

	return 0;
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

/**
 * sends a sign in message to the client, a sign in message
 * contains all other clients known to us(the member list)
 * and the size of this list
 *
 * @param arg contains an ip address to send the sign in message to
 */
void
send_sign_in (void * arg) {
	printf(CYN "#\t#\t#\t#\tsend_sign_in()\t#\t#\t#\t#\n" RESET);

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
		pthread_mutex_unlock(&mutex);
		return ;
	}

	//header of member list
	packet.version = VERSION; //version
	packet.typ = SIGN_IN; //type
	packet.length = htons(bufsize);
	packet.payload[0] = (uint8_t)List_no_of_members();

	int ip_addr_offset = 1;
	int id_offset;

	uint32_t ip_t;

//	memset(packet.payload, '\0', 20 * 20);

	//set content of member list
	for(i = 0; i < List_no_of_members(); i++) {
		ip_t = htonl(p->ip);
		//store in network byteorder
		packet.payload[0 + ip_addr_offset] = (ip_t & 0xFF000000) >> 24;
		packet.payload[1 + ip_addr_offset] = (ip_t & 0x00FF0000) >> 16;
		packet.payload[2 + ip_addr_offset] = (ip_t & 0x0000FF00) >> 8;
		packet.payload[3 + ip_addr_offset] = (ip_t & 0x000000FF);

		id_offset = 4 + ip_addr_offset;
		for(j = 0; j < ID_LENGTH; j++) {
			packet.payload[id_offset + j] = p->id[j];
			if (p->id[j] == '/0') {
				break;
			}
		}

		ip_addr_offset += SIZE_OF_MEMBER_IN_BYTES;
		p = p->next;
	}
	packet.crc = htonl(crc_32(packet.payload, bufsize));

	send_to_server(&packet, sock_fd);
	//flag for the update() bei recv_member_list()
	sign = true;

	if (close(sock_fd) != 0)
		perror(RED "Close: send_sign_in" RESET);
	pthread_mutex_unlock(&mutex);


}


/**
 * sends a quit message to all other clients known to us, a quit
 * message only contains our id
 */
void
send_quit (void * args) {
	printf(CYN "#\t#\t#\t#\tsend_quit()\t#\t#\t#\t#\n" RESET);
	struct packet packet;

	struct member *me = List_get_list();
	uint16_t size = strlen(me->id) + 1;
	//header of member list
	packet.version = VERSION; //version
	packet.typ = SIGN_OUT; //type
	packet.length = htons(size); //length
	packet.crc = htonl(crc_32(me->id, size));
//	memset(packet.payload, '\0', 20 * 20);
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

/**
 * sends a message(typed by the user)to a member on the list
 *
 * @param buffer contains the id and msg
 */
void
send_msg (void * buffer) {
	printf(CYN "#\t#\t#\t#\tsend_msg()\t#\t#\t#\t#\n" RESET);

	uint8_t* id = (uint8_t*)strtok(buffer, " \n\0"); //take id from buffer 
	id++; //remove @ from id
	uint8_t * msg = (uint8_t *)strtok(NULL, "\n\0"); //take msg from buffer

	uint16_t bufsize = strlen(msg) + 1;

	pthread_mutex_lock(&mutex);
	struct member messeger = List_search_member_id(id);
	int sock_fd;
	struct packet packet;

	if (messeger.ip == 0) {
		perror(RED "send_msg ID not found" RESET);
		pthread_mutex_unlock(&mutex);
		return ;
	}

	struct in_addr i_ip;
	i_ip.s_addr = messeger.ip;

	sock_fd = Client_connect(inet_ntoa(i_ip));

	if (sock_fd == -1) {
		perror(RED "Client_connect: send_msg" RESET);
		pthread_mutex_unlock(&mutex);
		return ;
	}

	//header of member list
	packet.version = VERSION; //version
	packet.typ = MESSAGE; //type
	packet.length = htons(bufsize);
	packet.crc = htonl(crc_32(msg, bufsize));

//	memset(packet.payload, '\0', 20 * 20);
	strcpy((char*)packet.payload, (char*)msg);

	send_to_server(&packet, sock_fd);

	if (close(sock_fd) != 0)
		perror(RED "Close: send_msg" RESET);
	pthread_mutex_unlock(&mutex);
}

/**
 * sends the member list to the client, the member list contains
 * all clients known to us
 *
 * @param arg contains the ip address to send the member list to
 */
void
send_member_list (void * arg) {
	printf(CYN "#\t#\t#\t#\tsend_member_list()\t#\t#\t#\t#\n" RESET);

	pthread_mutex_lock(&mutex);
	struct member *p = List_get_list();
	uint16_t bufsize = (List_no_of_members() * SIZE_OF_MEMBER_IN_BYTES) + 1;
	struct packet packet;
	uint32_t ip = *((uint32_t*)arg);

	int sock_fd;
	struct in_addr i_ip;
	i_ip.s_addr = ip;

	sock_fd = Client_connect(inet_ntoa(i_ip));

	if (sock_fd == -1) {
		perror(RED "Client_connect: send_member_list" RESET);
		pthread_mutex_unlock(&mutex);
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

	uint32_t ip_t;

//	memset(packet.payload, '\0', 20 * 20);

	//set content of member list
	for(i = 0; i < List_no_of_members(); i++) {
		ip_t = htonl(p->ip);
		//store in network byteorder
		packet.payload[0 + ip_addr_offset] = (ip_t & 0xFF000000) >> 24;
		packet.payload[1 + ip_addr_offset] = (ip_t & 0x00FF0000) >> 16;
		packet.payload[2 + ip_addr_offset] = (ip_t & 0x0000FF00) >> 8;
		packet.payload[3 + ip_addr_offset] = (ip_t & 0x000000FF);

		id_offset = 4 + ip_addr_offset;
		for(j = 0; j < ID_LENGTH; j++) {
			packet.payload[id_offset + j] = p->id[j];
		}
		printf("send_member_list: id: %s, ip: %d\n", p->id, p->ip);
		ip_addr_offset += SIZE_OF_MEMBER_IN_BYTES;
		p = p->next;
	}
	packet.crc = htonl(crc_32(packet.payload, bufsize));

	send_to_server(&packet, sock_fd);
	if (close(sock_fd) != 0)
		perror(RED "Close: send_member_list" RESET);
	pthread_mutex_unlock(&mutex);
}

/**
 * sends an error packet to the client, an error can occur if
 * 1. the client tried to connect to us but was already connected(error-code 0x0)
 * 2. if the length was invalid(error-code 0x1)
 * 3. if a client is not reachable(??) (error-code 0x2)
 *
 * @param buffer contains the ip and the error code to be send
 */
void
send_error(void *buffer) {
	printf(CYN "#\t#\t#\t#\tsend_error()\t#\t#\t#\t#\n" RESET);

	struct error_args* error_args = buffer;
	uint16_t string_length = htons(ERROR_CODE_LENGTH);
	struct packet packet;
	int sock_fd;

	//header of member list
	packet.version = VERSION; //version
	packet.typ = ERROR; //type
	packet.length = htons(string_length); //length
	packet.crc = htons(crc_32(&error_args->error_code, ERROR_CODE_LENGTH));
//	memset(packet.payload, '\0', 20 * 20);
	strcpy((char*)packet.payload, (char*)&error_args->error_code);

	sock_fd = Client_connect(error_args->ip);
	if (sock_fd == -1) {
		return ;
	}
	send_to_server(&packet, sock_fd);

	if (close(sock_fd) != 0)
		perror("Close: ");
}

/*
 * Sends to all of my members a list of new members
 * @param payload new list
 *
 */
void
send_update (uint8_t * payload) {
	printf(CYN "#\t#\t#\t#\tsend_update()\t#\t#\t#\t#\n" RESET);

	struct packet packet;
	struct member *p;
	struct in_addr i_ip;
	int sock_fd;

	uint16_t size = (payload[0] * SIZE_OF_MEMBER_IN_BYTES) + 1;

	int i;

	packet.version = VERSION; //version
	packet.typ = MEMBER_LIST; //type
	memcpy (packet.payload, payload, size);
	packet.crc = htonl(crc_32(packet.payload, size));
	packet.length = htons(size); //length

	printf("send_update: crc %u", crc_32(packet.payload, size));


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



/**							RECV_TASKS								*/

/**
 * Receives a list of new members
 *
 * @param payload -> list of new members
 * @param ip_addr ip -> adress from sender
 * @param sockfd -> socket where the transmission happend
 */
int
recv_sign_in (uint8_t * payload,
		const uint32_t ip_addr, int sockfd) {
	printf(CYN "#\t#\t#\t#\trecv_sign_in()\t#\t#\t#\t#\n" RESET);

	uint8_t no_member = payload[0];
	uint32_t ip;
	uint8_t *id;

	struct threadpool * send_pool;
	send_pool = Chat_get_sendpool();

	struct task_t job;

	int offset = 0;

	if (List_no_of_members() > 1) {
		send_update(payload);
	}

	int i;
	for (i = 0; i < no_member; i++) {
		ip = (uint32_t) payload[1 + offset] << 24
					| payload[2 + offset] << 16
					| payload[3 + offset] << 8
					| payload[4 + offset];

		ip = ntohl(ip);

		id = &payload[5 + offset];
		
		if (i == 0) {
			printf("recv_sing_in id: %s sock_fd %d\n", id, sockfd);	

			send_member_list(&ip);
		}

		pthread_mutex_lock(&mutex);
		if (List_new_member(id, ip) != 0) {
			printf(RED "recv_sign_in: id double %s or ip double %u\n" RESET, id, ip);

		}
		pthread_mutex_unlock(&mutex);

		printf(CYN "recv_sign_in id: %s\n" RESET, id);
		printf(CYN "recv_sign_in ip: %u\n\n" RESET, ip);

		offset += SIZE_OF_MEMBER_IN_BYTES;
	}

	return 0;
}

/**
 * Receives a quit from a id
 *
 * @param id -> id from quitter
 */

int
recv_quit (uint8_t *id) {
	printf(CYN "#\t#\t#\t#\trecv_quit()\t#\t#\t#\t#\n" RESET);

	printf("@%s: quit\n", id);

	pthread_mutex_lock(&mutex);
	if (List_delete_member(id) != 0) {
		perror(RED "recv_quit: id not found" RESET);
		return -1;
	}
	pthread_mutex_unlock(&mutex);

	return 0;
}

/**
 * Receives a msg from a id
 *
 * @param msg -> message
 * @param ip -> ip from messenger
 */

int
recv_msg (uint8_t *msg, const uint32_t ip) {
	printf(CYN "#\t#\t#\t#\trecv_msg()\t#\t#\t#\t#\n" RESET);

	struct member messeger;

	pthread_mutex_lock(&mutex);
	messeger = List_search_member_ip(ip);
	pthread_mutex_unlock(&mutex);
	
	printf(YEL "#%s: %s\n" RESET, messeger.id, msg);

	return 0;
}

/**
 * Receives a list of new members
 *
 * @param payload -> list of new members
 */

int
recv_member_list (uint8_t *payload) {
	printf(CYN "#\t#\t#\t#\trecv_member_list()\t#\t#\t#\t#\n" RESET);

	uint8_t no_member = payload[0];
	uint32_t ip;
	uint8_t *id;

	struct threadpool * send_pool;
	send_pool = Chat_get_sendpool();

	struct task_t job;

	int offset = 0;

	int i;

	if (List_no_of_members() > 1 && sign) {
		send_update(payload);
	}

	pthread_mutex_lock(&mutex);
	for (i = 0; i < no_member; i++) {
		ip = (uint32_t) payload[1 + offset] << 24
					| payload[2 + offset] << 16
					| payload[3 + offset] << 8
					| payload[4 + offset];

		ip = ntohl(ip);

		id = &payload[5 + offset];	

		if (List_new_member(id, ip) != 0) {
			printf(RED "recv_member_list: id double %s or ip double %u\n" RESET, id, ip);
//			struct error_args* error_args = malloc(sizeof(error_args));
//			struct task_t error_job;
//
//			error_args->ip[0] = ip & 0xFF000000;
//			error_args->ip[1] = ip & 0x00FF0000;
//			error_args->ip[2] = ip & 0x0000FF00;
//			error_args->ip[3] = ip & 0x000000FF;
//			error_args->error_code = ERROR_INVALID_ID;
//
//			error_job.routine_for_task = send_error;
//			error_job.arg = error_args;
//			error_job.mallfree = true;
//
//			Thpool_add_task(send_pool, error_job);
		}

		printf(CYN "recv_member_list id: %s\n" RESET, id);
		printf(CYN "recv_member_list ip: %u\n\n" RESET, ip);

		offset += SIZE_OF_MEMBER_IN_BYTES;
	}

	pthread_mutex_unlock(&mutex);

	sign = false;

	return 0;
}

int
recv_error(uint8_t *error, const uint32_t ip) {
	printf(CYN "#\t#\t#\t#\trecv_error()\t#\t#\t#\t#\n" RESET);

	struct member messenger;

	pthread_mutex_lock(&mutex);
	messenger = List_search_member_ip(ip);
	pthread_mutex_unlock(&mutex);

	printf(YEL "#%s: %s\n" RESET, messenger.id, error);

	return 0;
}

void
recv_from_client (void *sockfd) {
	packet_t pck;
	int num_bytes;

	int new_fd = *(int *)(sockfd);

	struct sockaddr_in client_ip;
	socklen_t addr_size = sizeof(client_ip);
	
	getpeername(new_fd,
			(struct sockaddr *)&client_ip,
			&addr_size);


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

		//checks for false crc and prints id + ip
		if(!crc_is_equal((uint8_t *)&pck.payload, ntohs(pck.length), ntohl(pck.crc))) {
			struct in_addr addr;
			addr.s_addr = client_ip.sin_addr.s_addr;
			printf(RED "false crc-sum %u on packet from %s, ip: %s\n" RESET,
					pck.crc, List_search_member_ip(client_ip.sin_addr.s_addr).id , inet_ntoa(addr));
		}
				
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


/**
 * return 1 if crcToCheck and crc computed with strToCheck are equal else
 * return 0
 */
int crc_is_equal(uint8_t* strToCheck, uint16_t strLength, uint32_t crcToCheck) {
	uint crc = crc_32(strToCheck, strLength);
	printf("recv_from_client, crc: %u \n", crc);
	return crc == crcToCheck;
}




#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "list.h"

member_t * list = NULL;
int counter = 0;

int
init_list (const char id[16]) {

	if (list != NULL) {
		errno = EADDRINUSE;
		return errno;
	}

	list = malloc(sizeof(member_t));

	if (list == NULL) {
		errno = ENOMEM;
		return errno;
	}

	memcpy(list->id, id, strlen(id)+1);
	list->port = 6100;
	list->next = NULL;
	counter++;

	return 0;
}

int
list_set_first_ip(void) {
	struct ifreq ifr;
	strcpy(ifr.ifr_name, "wlp3s0");
	
	if (ioctl(*list->sock_fd, SIOCGIFADDR, &ifr) != 0) {
		perror("ioctl: ");
		return -1;
	}

	struct sockaddr_in * my_ip = (struct sockaddr_in *) &ifr.ifr_addr;

	list->ip =  my_ip->sin_addr.s_addr;

	return 0;

}

int
new_member (const char id[16], const uint32_t ip,
				const uint16_t port, int * socket) {
	member_t * new_member = NULL;
	member_t * p;

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return errno;
	}


	for (p = list; p->next != NULL; p = p->next){
		if ((strcmp(id, p->id)) == 0) {
			break;
		}
	}

	if (p != NULL) {
		errno = EPERM;
		return errno;
	}

	new_member = malloc(sizeof(member_t));

	if (new_member == NULL) {
		errno = ENOMEM;
		return errno;
	} 
	memcpy(list->id, id, strlen(id)+1);
	new_member->ip = ip;
	new_member->port = port;
	new_member->sock_fd = socket;
	new_member->next = NULL;

	for (p = list; p != NULL; p = p->next);

	p = new_member;
	counter++;

	return 0;
	
}

struct member
search_member_id (const char id[16]) {
	struct member * p = NULL;
	member_t search;

	memset(&search, 0, sizeof(member_t));	

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return search;
	}

	for (p = list; p->next != NULL; p = p->next){
		if ((strcmp(id, p->id)) == 0) {
			search = *p;
			return search;
		}
	}

	return search;
}

struct member
search_member_ip (const uint32_t ip) {
	struct member * p = NULL;
	member_t search;

	memset(&search, 0, sizeof(member_t));	

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return search;
	}

	for (p = list; p->next != NULL; p = p->next){
		if (p->ip == ip) {
			search = *p;
			return search;
		}
	}

	return search;
}

int
delete_member (const char id[16]) {
	member_t * del_member = NULL;
	member_t * p = NULL;
	
	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return -1;
	}

	for (p = list; p->next != NULL; p = p->next){
		if (p->id == id) {
			del_member = p;
		}
	}

	if (del_member == NULL) {
		errno = EADDRNOTAVAIL;
		return -1;
	}

	p->next = del_member->next;
	free(del_member);
	counter--;

	return 0;
	
}

int
delete_list (void) {
	member_t * p = NULL;
	member_t * previous = NULL;

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return errno;
	}

	p = list;
	while(p != NULL) {
		previous = p;
		p = p->next;
		free(previous);
	}
	counter = 0;

	return 0;
}

void
print_members (void) {
	member_t * p = NULL;

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return ;
	}

	printf("_____________MEMBER_LIST____________\n\n");

	for (p = list; p->next != NULL; p = p->next){
		printf("id________%s_\n", p->id);
		printf("ip________%d_\n", p->ip);
		printf("port______%d_\n", p->port);
		printf("socket____%d_\n", *p->sock_fd);
		printf("\n");
	}

}

int *
get_socket_by_ip (const uint32_t ip) {
	member_t * p = NULL;

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return NULL;
	}

	for (p = list; p->next != NULL; p = p->next){
		if (p->ip == ip) {
			break;
		}
	}

	if (p != NULL) {
		return p->sock_fd;
	}

	return NULL;
	
}

int *
get_socket_by_id (const char id[16]) {
	member_t * p = NULL;

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return NULL;
	}

	for (p = list; p->next != NULL; p = p->next){
		if (p->id == id) {
			break;
		}
	}

	if (p != NULL) {
		return p->sock_fd;
	}

	return NULL;
	
}

int
number_of_members(void) {
	return counter;

}


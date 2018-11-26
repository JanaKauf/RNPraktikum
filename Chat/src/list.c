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

member_t *list = NULL;
int counter = 0;

int
List_init (uint8_t id[16], uint32_t ip) {

	list = malloc(sizeof(member_t));

	if (list == NULL) {
		return -1;
	}

	strcpy(list->id, id);
	list->ip =  ip;
	list->next = NULL;
	counter++;

	return 0;
}

int
List_new_member (uint8_t id[16], uint32_t ip) {
	member_t *new_member = NULL;
	member_t *p;

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return -1;
	}


	for (p = list; p->next != NULL; p = p->next){
		if ((strncmp(id, p->id, 16)) == 0) {
			errno = EPERM;
			return -1;
		}
	}

	new_member = malloc(sizeof(member_t));

	if (new_member == NULL) {
		errno = ENOMEM;
		return -1;
	}

	strcpy(new_member->id, id);
	new_member->ip = ip;
	new_member->sock_fd = NULL;
	new_member->next = NULL;

	p->next = new_member;
	counter++;

	return 0;
	
}

struct member
List_search_member_id (uint8_t id[16]) {
	struct member *p = NULL;
	member_t search;

	memset(&search, 0, sizeof(member_t));	

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return search;
	}

	for (p = list; p->next != NULL; p = p->next){
		if (strncmp(id, p->id, 16) == 0) {
			search = *p;
			return search;
		}
	}

	return search;
}

struct member
List_search_member_ip (uint32_t ip) {
	struct member *p = NULL;
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
List_delete_member (uint8_t id[16]) {
	member_t *del_member = NULL;
	member_t *p = NULL;
	
	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return -1;
	}

	for (p = list; p->next != NULL; p = p->next){
		if (strncmp(p->id, id, 16) == 0) {
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
List_delete (void) {
	member_t *p = NULL;
	member_t *previous = NULL;

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return -1;
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
List_print (void) {
	member_t *p = NULL;
	struct in_addr ip_addr;

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return ;
	}

	printf("_____________MEMBER_LIST____________\n\n");

	for (p = list; p != NULL; p = p->next){
		ip_addr.s_addr = p->ip;
		printf("id\t\t%s\n", p->id);
		printf("ip\t\t%s\n", inet_ntoa(ip_addr));
		printf("sockfd\t\t%d\n", p->sock_fd);
		printf("\n");
	}

}

int *
List_get_sockfd_by_id (uint8_t id[16]) {
	member_t *p = NULL;

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return NULL;
	}

	for (p = list; p->next != NULL; p = p->next){
		if (strncmp(p->id, id, 16) == 0) {
			break;
		}
	}

	if (p != NULL) {
		return p->sock_fd;
	}

	return NULL;
	
}

int *
List_get_sockfd_by_ip (uint32_t ip) {
	member_t *p = NULL;

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

struct member* List_get_ip_by_sockfd(int sockfd) {
	member_t *p = NULL;

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return NULL;
	}

	for (p = list; p->next != NULL; p = p->next){
		if (*p->sock_fd == sockfd) {
			break;
		}
	}

	return p;
}


int
List_no_of_members(void) {
	return counter;

}

struct member *
List_get_list (void) {
	return list;

}


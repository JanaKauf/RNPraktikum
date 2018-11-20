#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "list.h"

member_t * list = NULL;
int counter = 0;

int
init_list (const char id[16], const uint32_t ip,
				const uint16_t port) {
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
	list->ip = ip;
	list->port = port;
	list->next = NULL;
	counter++;

	return 0;
}

int
new_member (const char id[16], const uint32_t ip,
				const uint16_t port) {
	struct member * new_member = NULL;
	struct member * p = NULL;

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return errno;
	}

	if ((p = search_member(id)) != NULL) {
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
	new_member->next = NULL;

	for (p = list; p != NULL; p = p->next);

	p = new_member;
	counter++;

	return 0;
	
}

struct member *
search_member (const char id[16]) {
	struct member * p = NULL;

	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return NULL;
	}

	for (p = list; p->next != NULL; p = p->next){
		if ((strcmp(id, p->id)) == 0) {
			return p;
		}
	}

	return NULL;
}

int
delete_member (const char id[16]) {
	struct member * del_member = NULL;
	struct member * p = NULL;
	
	if (list == NULL) {
		errno = EADDRNOTAVAIL;
		return errno;
	}

	del_member = search_member(id);

	if (del_member == NULL) {
		errno = EADDRNOTAVAIL;
		return errno;
	}

	p->next = del_member->next;
	free(del_member);
	counter--;

	return 0;
	
}

int
delete_list (void) {
	struct member * p = NULL;
	struct member * previous = NULL;

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

int
number_of_members(void) {
	return counter;

}


#ifndef _LIST_H
#define _LIST_H

#define SIZE_OF_HEADER_IN_BYTES 8
#define SIZE_OF_MEMBER_IN_BYTES 20//((sizeof char) * 16) + sizeof(uint32)
#define ID_LENGTH 16

#include <stdint.h>

typedef struct member {
	uint32_t	ip;
	char 		id[16];
	int *		sock_fd;
	struct member *	next;
} member_t;

extern int init_list (const char id[16]);

extern int list_set_first_ip(void);

extern int new_member (const char id[16],
		const uint32_t ip,
		int * socket);

extern struct member search_member_id (const char id[16]);

extern struct member search_member_ip (const uint32_t ip);

extern int delete_member (const char id[16]);

extern int delete_list (void);

extern void print_members (void);

extern int * get_socket_by_ip (const uint32_t ip);

extern int * get_socket_by_id (const char id[16]);

extern int number_of_members (void);

#endif /* _LIST_H */

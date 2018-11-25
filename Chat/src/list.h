#ifndef _LIST_H
#define _LIST_H

#define SIZE_OF_HEADER_IN_BYTES 8
#define SIZE_OF_MEMBER_IN_BYTES 20//((sizeof char) * 16) + sizeof(uint32)
#define ID_LENGTH 16
#define ID_NAME "Raupe\0";
#define INTERFACE_NAME "wlp1s0"

#include <stdint.h>

typedef struct member {
	uint32_t	ip;
	uint8_t 	id[16];
	int			*sock_fd;
	struct member *next;
} member_t;

extern int List_init (uint8_t id[16], uint32_t ip);

extern int List_new_member (uint8_t id[16],
		const uint32_t ip,
		int *sockfd);

extern struct member List_search_member_id (uint8_t id[16]);

extern struct member List_search_member_ip (uint32_t ip);

extern int List_delete_member (uint8_t id[16]);

extern int List_delete (void);

extern void List_print (void);

extern int* List_get_sockfd_by_ip (uint32_t ip);

extern int* List_get_sockfd_by_id (uint8_t id[16]);

extern int List_no_of_members (void);

extern struct member * List_get_list(void);

#endif /* _LIST_H */

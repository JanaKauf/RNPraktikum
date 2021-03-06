#ifndef _LIST_H
#define _LIST_H

#include <stdint.h>

typedef struct member {
	uint32_t	ip;
	uint8_t 	id[16];
	struct member *next;
} member_t;

extern int List_init (uint8_t id[16], uint32_t ip);

extern int List_new_member (uint8_t id[16],
		const uint32_t ip);

extern struct member List_search_member_id (uint8_t id[16]);

extern struct member List_search_member_ip (uint32_t ip);

extern int List_delete_member (uint8_t id[16]);

extern int List_delete (void);

extern void List_print (void);

extern int List_no_of_members (void);

extern struct member * List_get_list(void);

#endif /* _LIST_H */

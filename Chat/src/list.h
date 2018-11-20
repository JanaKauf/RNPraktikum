#ifndef _LIST_H
#define _LIST_H
#include <stdint.h>
#include <time.h>

typedef struct member {
	char 			id[16];
	uint32_t		ip;
	uint16_t		port;
	struct member*	next;
} member_t;


extern int init_list (const char id[16],
		const uint32_t ip,
		const uint16_t port);

extern int new_member (const char id[16],
		const uint32_t ip,
		const uint16_t port);

extern struct member * search_member (const char id[16]);

extern int delete_member (const char id[16]);

extern int delete_list (void);

extern int number_of_members (void);

#endif /* _LIST_H */

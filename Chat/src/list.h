#ifndef _LIST_H
#define _LIST_H
#include <stdint.h>
#include <time.h>
#define SIZE_OF_HEADER_IN_BYTES 8
#define SIZE_OF_MEMBER_IN_BYTES ((sizeof char) * 16) + sizeof(uint32)
#define ID_LENGTH 16; //length of id without \0



typedef struct member {
	char 			id[16];
	uint32_t		ip;

	struct member*	next;
} member_t;


extern int init_list (const char id[16],
		const uint32_t ip,
		const uint16_t port);

extern int new_member (const char id[16],
		const uint32_t ip,
		const uint16_t port);

extern struct member * search_member_id (const char id[16]);

extern struct member * search_member_ip (const uint32_t ip);

extern int delete_member (const char id[16]);

extern int delete_list (void);

extern int number_of_members (void);

#endif /* _LIST_H */

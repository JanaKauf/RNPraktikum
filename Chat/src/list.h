#ifndef _LIST_H
#define _LIST_H
#include <stdint.h>
#include <time.h>

typedef struct member {
	char 			id[16];
	uint32_t		ip;
	uint16_t		port;
	time_t			time_stamp;
	struct member*	next;
} member_t;


int init_list (const char id[16],
		const uint32_t ip,
		const uint16_t port,
		const time_t time_stamp);
int new_member (const char id[16],
		const uint32_t ip,
		const uint16_t port,
		const time_t time_stamp);
struct member * search_member (const char id[16]);
int delete_member (const char id[16]);
int delete_list (void);

#endif // _LIST_H

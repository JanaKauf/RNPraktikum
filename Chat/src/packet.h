#ifndef _PACKET_H
#define _PACKET_H
#include <std/int.h>

typedef struct packet {
	uint8_t version;
	uint8_t typ;
	uint16_t length;
	uint32_t crc;
	void *payload;

}packet_t;

#endif /* _PACKET_H */

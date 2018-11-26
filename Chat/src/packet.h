#ifndef _PACKET_H
#define _PACKET_H

typedef struct packet {
	uint8_t version;
	uint8_t typ;
	uint16_t length;
	uint32_t crc;
	uint8_t payload[20 * 20];

}packet_t;

#endif /* _PACKET_H */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>


void
recv_sign_in (void * buffer) {
	uint32_t crc;
	uint16_t length;
	char * buf = buffer;

	length = (uint16_t) buf[2] << 8 | buf[3];
	crc = (uint32_t) buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];
}

void
recv_quit (void * buffer) {
	uint32_t crc;
	uint16_t length;
	char * buf = buffer;

	length = (uint16_t) buf[2] << 8 | buf[3];
	crc = (uint32_t) buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];
}

void
recv_msg (void * buffer) {
	uint32_t crc;
	uint16_t length;
	char * buf = buffer;

	length = (uint16_t) buf[2] << 8 | buf[3];
	crc = (uint32_t) buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];
}

void
recv_member_list (void * buffer) {
	uint32_t crc;
	uint16_t length;
	char * buf = buffer;

	length = (uint16_t) buf[2] << 8 | buf[3];
	crc = (uint32_t) buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];

}

void
recv_error(void *buffer) {
	uint32_t crc;
	uint16_t length;
	char * buf = buffer;

	length = (uint16_t) buf[2] << 8 | buf[3];
	crc = (uint32_t) buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];
}

void
send_sign_in (void * buffer) {
}

void
send_quit (void * buffer) {
}

void
send_msg (void * buffer) {
}

void
send_member_list (void * buffer) {
}

void
send_error(void *buffer) {
}

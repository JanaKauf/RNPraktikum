/*
 * server.h
 *
 *  Created on: Nov 16, 2018
 *      Author: networker
 */

#ifndef SRC_SERVER_H_
#define SRC_SERVER_H_

#include <stdint.h>

#define PORT "6100"
#define HOLD_QUEUE 20

extern void * Server_thread (void *args);
extern int get_my_ip(uint32_t* ip);
#endif /* SRC_SERVER_H_ */

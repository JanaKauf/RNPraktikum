/*
 * server.h
 *
 *  Created on: Nov 16, 2018
 *      Author: networker
 */

#ifndef SRC_SERVER_H_
#define SRC_SERVER_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>


#define PORT "6100"


void *get_in_address(struct sockaddr *sa);
int start_server();


#endif /* SRC_SERVER_H_ */

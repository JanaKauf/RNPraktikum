#ifndef _TASK_H
#define _TASK_H

#define SIGN_IN 1
#define SIGN_OUT 2
#define MEMBER_LIST 3
#define MESSAGE 4
#define ERROR 5

#define VERSION 1
//###################CONNECT_TASKS#######################
struct args_connect{
	char * ip;
	int * sock_fd;
};

extern void connect_to_server (void*);
extern void disconnect_from_server (void * sockfd);

//###################SEND_TASKS#######################
struct args_send{
	void * buf;
	int * sock_fd;
};

extern void send_sign_in (void * buffer);

//###################RECV_TASKS#######################

extern void recv_from_client(void * sockfd);

/*
extern void recv_quit(void * buf);
extern void recv_msg(void * buf);
extern void recv_member_list(void * buff);
extern void recv_error(void *buff);
extern void send_sign_in(void * buff);
extern void send_quit(void * buff);
extern void send_msg(void * buff);
extern void send_member_list(void * buff);
extern void send_error(void *buff);
*/

#endif /* _TASK_H */

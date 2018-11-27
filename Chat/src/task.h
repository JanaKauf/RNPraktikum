#ifndef _TASK_H
#define _TASK_H

#define SIGN_IN 1
#define SIGN_OUT 2
#define MEMBER_LIST 3
#define MESSAGE 4
#define ERROR 5

#define VERSION 1
//###################SEND_TASKS#######################
extern void send_sign_in (void * arg);
extern void send_quit(void * args);
extern void send_msg(void * buff);
extern void send_member_list(void * buff);
extern void send_error(void *buff);
extern void resend_msg(void * arg);
//###################RECV_TASKS#######################

extern void recv_from_client(void * sockfd);

/*
extern void recv_quit(void * buf);
extern void recv_msg(void * buf);
extern void recv_member_list(void * buff);
extern void recv_error(void *buff);
extern void send_sign_in(void * buff);
*/

#endif /* _TASK_H */

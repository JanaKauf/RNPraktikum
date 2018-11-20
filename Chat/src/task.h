#ifndef _TASK_H
#define _TASK_H

extern void recv_sign_in(void * buf);
extern void recv_sign_up(void * buf);
extern void recv_msg(void * buf);
extern void recv_member_list(void * buff);
extern void recv_error(void *buff);
extern void send_sign_in(void * buff);
extern void send_sign_up(void * buff);
extern void send_msg(void * buff);
extern void send_member_list(void * buff);
extern void send_error(void *buff);

#endif /* _TASK_H */

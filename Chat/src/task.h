#ifndef _TASK_H
#define _TASK_H

extern int recv_sign_in(void * buff);
extern int recv_sign_up(void * buff);
extern int recv_msg(void * buff);
extern int recv_member_list(void * buff);
extern int recv_error(void *buff);
extern int send_sign_in(void * buff);
extern int send_sign_up(void * buff);
extern int send_msg(void * buff);
extern int send_member_list(void * buff);
extern int send_error(void *buff);

#endif /* _TASK_H */

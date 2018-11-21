#ifndef _TASK_H
#define _TASK_H


//###################CONNECT_TASKS#######################

void connect_to_server (char * server_ip, void * new_socket);
void disconnect_from_server (void * socket);

//###################SEND_TASKS#######################


//###################RECV_TASKS#######################

extern void recv_from_client(void * socket);

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

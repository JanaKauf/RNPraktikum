
#ifndef _LIST_THRSAFE_H
#define _LIST_THRSAFE_H


extern int init_thrsafe(void);

extern int thrsafe_new_member(const char id[16],
							const uint32_t ip,
							const uint16_t port,
							int * socket);

extern int thrsafe_delete_member_id (const char id[16]);

extern int thrsafe_set_socket_id (const char id[16], int * socket);

extern int thrsafe_set_socket_ip (const uint32_t ip, int * socket);

extern int thrsafe_clean (void);

#endif /* _LIST_THRSAFE_H */

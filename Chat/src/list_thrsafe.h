
#ifndef _LIST_THRSAFE_H
#define _LIST_THRSAFE_H


extern int Thrsafe_init(void);

extern int Thrsafe_new_member(const char id[16],
							const uint32_t ip,
							int * socket);

extern int Thrsafe_delete_member_id (const char id[16]);

extern int Thrsafe_set_socket_id (const char id[16], int *socket);

extern int Thrsafe_set_socket_ip (const uint32_t ip, int *socket);

extern int Thrsafe_clean (void);

#endif /* _LIST_THRSAFE_H */

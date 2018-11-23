
#ifndef _LIST_THRSAFE_H
#define _LIST_THRSAFE_H


extern int Thrsafe_init(void);

extern int Thrsafe_new_member(const char id[16],
							const uint32_t ip,
							int * sockfd);

extern int Thrsafe_delete_member_id (const char id[16]);

extern int Thrsafe_set_sockfd_id (const char id[16], int *sockfd);

extern int Thrsafe_set_sockfd_ip (const uint32_t ip, int *sockfd);

extern int Thrsafe_clean (void);

#endif /* _LIST_THRSAFE_H */

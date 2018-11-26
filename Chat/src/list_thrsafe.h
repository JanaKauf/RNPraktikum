
#ifndef _LIST_THRSAFE_H
#define _LIST_THRSAFE_H


extern int Thrsafe_init(void);

extern int Thrsafe_new_member(uint8_t id[16],
							uint32_t ip);

extern int Thrsafe_delete_member_id (uint8_t id[16]);

extern int Thrsafe_set_sockfd_id (uint8_t id[16], int *sockfd);

extern int Thrsafe_set_sockfd_ip (uint32_t ip, int *sockfd);

extern int Thrsafe_clean (void);

#endif /* _LIST_THRSAFE_H */

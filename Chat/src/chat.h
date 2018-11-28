#ifndef _CHAT_H
#define _CHAT_H

extern struct threadpool * Chat_get_recvpool (void);
extern struct threadpool * Chat_get_sendpool (void);

#endif /* _CHAT_H */

#ifndef CLIENT_H
#define CLIENT_H

#define PORT "6100"

extern int * Client_connect (char * server_ip);
extern void Client_disconnect (void);
#endif /* CLIENT_H*/

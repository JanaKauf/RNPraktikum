#ifndef CLIENT_H
#define CLIENT_H

#define PORT "6100"

extern void Client_protocol (char *protocol);
extern int Client_connect (char *server_ip);
#endif /* CLIENT_H*/

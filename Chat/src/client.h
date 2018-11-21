#ifndef CLIENT_H
#define CLIENT_H

#define PORT "6100"

extern int client_connect (char * server_ip);
extern int client_send (int * socket, char * buf);

#endif /* CLIENT_H*/

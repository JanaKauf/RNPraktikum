#include <stdio.h>
#include "list.h"
#include <pthread.h>
#include <errno.h>

pthread_mutex_t mutex;

int
init_thrsafe (void) {

	init_list("Raupe\0");

	pthread_mutex_init(&mutex, NULL);
	return errno;
}

int
thrsafe_new_member(const char id[16], const uint32_t ip,
				const uint16_t port, int * socket) {

	pthread_mutex_lock(&mutex);

	errno = new_member(id, ip, port, socket);

	pthread_mutex_unlock(&mutex);

	return errno;

}

int
thrsafe_delete_member_id (const char id[16]) {
	member_t * search = NULL;

	pthread_mutex_lock(&mutex);

	errno = delete_member(id);

	pthread_mutex_unlock(&mutex);

	return errno;

}

int
thrsafe_set_socket_id (const char id[16], int * socket) {
	int * sock = get_socket_by_id(id);

	if (sock == NULL) {
		return -1;
	}

	pthread_mutex_lock(&mutex);

	sock = socket;

	pthread_mutex_unlock(&mutex);

}

int
thrsafe_set_socket_ip (const uint32_t ip, int * socket) {
	int * sock = get_socket_by_ip(ip);

	if (sock == NULL) {
		return -1;
	}

	pthread_mutex_lock(&mutex);

	sock = socket;

	pthread_mutex_unlock(&mutex);

}

int
thrsafe_clean (void) {
	delete_list();
	pthread_mutex_destroy(&mutex);

	return errno;

}

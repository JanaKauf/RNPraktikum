#include <stdio.h>
#include "list.h"
#include <pthread.h>
#include <errno.h>

pthread_mutex_t mutex;

int
init_thrsafe (void) {

	if (errno = init_list("Raupe\0") != 0){
		perror("init_thrsafe: init_list");		
		return -1;
	}

	if (pthread_mutex_init(&mutex, NULL) != 0) {
		perror("init_thrsafe: mutex_init");
		return -1;
	}
	return 0;
}

int
thrsafe_new_member(const char id[16], const uint32_t ip, int * socket) {
	int err = 0;

	pthread_mutex_lock(&mutex);

	err = new_member(id, ip, socket);

	pthread_mutex_unlock(&mutex);

	return err;

}

int
thrsafe_delete_member_id (const char id[16]) {
	member_t * search = NULL;
	int err = 0;

	pthread_mutex_lock(&mutex);

	err = delete_member(id);

	pthread_mutex_unlock(&mutex);

	return err;

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

	if (delete_list()) {
		perror("thrsafe_clean: ");
		return -1;
	}
	pthread_mutex_destroy(&mutex);

	return errno;

}
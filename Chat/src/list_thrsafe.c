#include <stdio.h>
#include "list.h"
#include <pthread.h>
#include <errno.h>

pthread_mutex_t mutex;

int
Thrsafe_init (void) {

	if (errno = List_init(ID_NAME) != 0){
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
Thrsafe_new_member(const char id[16], const uint32_t ip, int * sockfd) {
	int err = 0;

	pthread_mutex_lock(&mutex);

	err = List_new_member(id, ip, sockfd);

	pthread_mutex_unlock(&mutex);

	return err;

}

int
Thrsafe_delete_member_id (const char id[16]) {
	int err = 0;

	pthread_mutex_lock(&mutex);

	err = List_delete_member(id);

	pthread_mutex_unlock(&mutex);

	return err;

}

int
Thrsafe_set_sockfd_id (const char id[16], int *sockfd) {
	int *sock_fd = List_get_sockfd_by_id(id);

	if (sock_fd == NULL) {
		return -1;
	}

	pthread_mutex_lock(&mutex);

	sock_fd = sockfd;

	pthread_mutex_unlock(&mutex);

}

int
Thrsafe_set_sockfd_ip (const uint32_t ip, int *sockfd) {
	int *sock_fd = List_get_sockfd_by_ip(ip);

	if (sock_fd == NULL) {
		return -1;
	}

	pthread_mutex_lock(&mutex);

	sock_fd = sockfd;

	pthread_mutex_unlock(&mutex);

}

int
Thrsafe_clean (void) {

	if (List_delete()) {
		perror("thrsafe_clean: ");
		return -1;
	}
	pthread_mutex_destroy(&mutex);

	return errno;

}

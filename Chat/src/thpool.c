#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include "taskqueue.h"
#include "thpool.h"

struct threadpool *
thpool_create(const mqd_t taskqueue) {
    int i;
    struct threadpool * pool;

    if ((pool = malloc(sizeof(struct threadpool))) == NULL) {
        errno = EADDRNOTAVAIL;
        perror("thpool: thpool_create - allocate_pool");
        goto err;
    }

    if ((pool->threads = malloc(sizeof(pthread_t) * NUM_THREADS)) == NULL) {
        errno = EADDRNOTAVAIL;
        perror("thpool: thpool_create - allocate_threads");
        goto err;
    }

    pool->taskqueue = taskqueue;

	if ((errno = pthread_mutex_init(&(pool->mutex), NULL)) != 0)
	   perror("thpool: pthread_mutex_init");	

    if (errno) {
        goto err;
    }

    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&(pool->threads[i]), NULL,
                           thpool_routine, (void *) pool) != 0) {
            thpool_destroy(pool);
            return NULL;
        }
   }

    return pool;

    err:
        if (pool) {
            thpool_free(pool);
        }
        return NULL;
}

int
thpool_add_task (struct threadpool * pool, const struct task job,
					const int prio) {
    if (pool == NULL) {
        errno = EADDRNOTAVAIL;
        perror("Invalid Pool");
        return errno;
    }

	if((errno = pthread_mutex_lock(&(pool->mutex))) != 0)
		perror("thpool: pthread_mutex_lock");

	if((errno = pthread_mutex_unlock(&(pool->mutex))) != 0)
		perror("thpool: pthread_mutex_unlock");

	if((errno = taskqueue_send(pool->taskqueue, job, prio, true)) != 0)
		perror("thpool: sendToTaskQueue");

    return 0;
}

void *
thpool_routine(void * threadpool) {
    struct threadpool *pool = (struct threadpool *) threadpool;
    struct task job;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    while(true) {

		if((errno = pthread_mutex_lock(&(pool->mutex))) != 0)
			perror("thpool: pthread_mutex_lock");

		if((errno = pthread_mutex_unlock(&(pool->mutex))) != 0)
			perror("thpool: pthread_mutex_unlock");

        job = taskqueue_receive(pool->taskqueue);

        (*(job.routine_for_task))(job.arg);

    }

    pthread_exit(0);
    return NULL;
}

int
thpool_destroy(struct threadpool * pool) {
    int i;

    if (pool == NULL) {
        errno = EADDRNOTAVAIL;
        perror("thpool: thpool_destroy - invalid_pool");
        return errno;
    }

	if((errno = pthread_mutex_lock(&(pool->mutex))) != 0)
		perror("thpool: pthread_mutex_lock");

    if (errno) {
        return errno;
    }

	if((errno = pthread_mutex_unlock(&(pool->mutex))) != 0)
		perror("thpool: pthread_mutex_unlock");

    if (errno) {
        return errno;
    }

    for (i = 0; i < NUM_THREADS; i++) {
		if((errno = pthread_cancel(pool->threads[i])) != 0)
			perror("thpool: pthread_cancel");
    }

    taskqueue_close(pool->taskqueue);

    for (i = 0; i < NUM_THREADS; i++) {
		if ((errno = pthread_join(pool->threads[i], NULL)))
			perror("thpool: pthread_join");
        }

    if (!errno) {
        thpool_free(pool);
    }

    return errno;
}

int
thpool_free (struct threadpool * pool) {
    if (pool == NULL) {
        errno = EADDRNOTAVAIL;
        perror("thpool: thpool_free - invalid_pool");
        return errno;
    }

    if (pool->threads) {
        free(pool->threads);
		if((errno = pthread_mutex_destroy(&(pool->mutex))) != 0)
			perror("thpool: pthread_mutex_destroy");
    }

    free(pool);
    return 0;
}


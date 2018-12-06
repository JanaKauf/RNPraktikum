#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include "thpool.h"

struct threadpool *
Thpool_create() {
    int i;
    struct threadpool *pool;

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

    pool->tasks = NULL;

	pool->counter = 0;


	if ((errno = pthread_cond_init(&(pool->cond), NULL)) != 0)
	   perror("thpool: pthread_mutex_init");	

	if ((errno = pthread_mutex_init(&(pool->mutex), NULL)) != 0)
	   perror("thpool: pthread_mutex_init");	

    if (errno) {
        goto err;
    }

    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&(pool->threads[i]), NULL,
                           Thpool_routine, (void *) pool) != 0) {
            Thpool_destroy(pool);
            return NULL;
        }
   }

    return pool;

    err:
        if (pool) {
            Thpool_free(pool);
        }
        return NULL;
}

int
Thpool_add_task (struct threadpool *pool, const struct task_t job) {
    if (pool == NULL) {
        errno = EADDRNOTAVAIL;
        perror("Invalid Pool");
        return errno;
    }

	struct task_t *new = malloc(sizeof(struct task_t));
	new->routine_for_task = job.routine_for_task;
	new->arg = job.arg;
	new->next = NULL;


	struct task_t *p;


	if((errno = pthread_mutex_lock(&(pool->mutex))) != 0)
		perror("thpool: pthread_mutex_lock");

	if (pool->tasks == NULL) {
		pool->tasks = new;
		pool->counter++;

		if ((pthread_cond_signal(&(pool->cond))) != 0)
			perror("thpool: pthread_cond_broadcast");

		if((errno = pthread_mutex_unlock(&(pool->mutex))) != 0)
			perror("thpool: pthread_mutex_unlock");

		return 0;
	}

	for (p = pool->tasks; p->next != NULL; p = p->next) {
	
	}

	p->next = new;
	pool->counter++;
	

	if ((pthread_cond_signal(&(pool->cond))) != 0)
		perror("thpool: pthread_cond_broadcast");

	if((errno = pthread_mutex_unlock(&(pool->mutex))) != 0)
		perror("thpool: pthread_mutex_unlock");

    return 0;
}

static void
cleanup_handler (void *args) {
	printf("Cleaning...\n");

	pthread_mutex_t * mutex = (pthread_mutex_t *)args;

	if (pthread_mutex_unlock(mutex)) 
		perror("Thpool: mutex_unlock");
}

void *
Thpool_routine(void *threadpool) {
    struct threadpool *pool = (struct threadpool *) threadpool;
	struct task_t *first;
    struct task_t job;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    while(true) {

		if((errno = pthread_mutex_lock(&(pool->mutex))) != 0)
			perror("thpool: pthread_mutex_lock");

		pthread_cleanup_push(cleanup_handler, &(pool->mutex));
		while (pool->counter == 0) {
			if ((errno =pthread_cond_wait(&(pool->cond), &(pool->mutex))) != 0)
				perror("thpool: pthread_cond_wait");

		}
		pthread_cleanup_pop(0);

		first = pool->tasks;

		job = *first;

		pool->tasks = pool->tasks->next;
		pool->counter--;

		if((errno = pthread_mutex_unlock(&(pool->mutex))) != 0)
			perror("thpool: pthread_mutex_unlock");

        (*(job.routine_for_task))(job.arg);

		free(first->arg);
		free(first);

    }

    pthread_exit(0);
    return NULL;
}

int
Thpool_destroy(struct threadpool *pool) {
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

    for (i = 0; i < NUM_THREADS; i++) {
		if ((errno = pthread_join(pool->threads[i], NULL)))
			perror("thpool: pthread_join");
        }

    if (!errno) {
        Thpool_free(pool);
    }

    return errno;
}

int
Thpool_free (struct threadpool *pool) {
    if (pool == NULL) {
        errno = EADDRNOTAVAIL;
        perror("thpool: thpool_free - invalid_pool");
        return errno;
    }

    if (pool->threads) {
        free(pool->threads);
		if((errno = pthread_mutex_destroy(&(pool->mutex))) != 0)
			perror("thpool: pthread_mutex_destroy");
		pthread_cond_destroy(&(pool->cond));
    }

    free(pool);
    return 0;
}


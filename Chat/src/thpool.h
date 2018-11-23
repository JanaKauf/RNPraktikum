#ifndef _THPOOL_H
#define _THPOOL_H
#include "taskqueue.h"
#include <pthread.h>

#define NUM_THREADS 15
#define NUM_TASKS	15

struct threadpool {
    pthread_mutex_t mutex;
    pthread_t * threads;
    mqd_t taskqueue;
};

extern struct threadpool* Thpool_create(mqd_t taskqueue);

extern int Thpool_add_task(struct threadpool *pool,
							struct task job,
							int prio);

void * Thpool_routine(void *threadpool);

extern int Thpool_destroy(struct threadpool *pool);

extern int Thpool_free(struct threadpool *pool);

#endif /* _THPOOL_H*/

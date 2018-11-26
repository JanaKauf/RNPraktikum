#ifndef _THPOOL_H
#define _THPOOL_H
//#include "taskqueue.h"
#include <pthread.h>

#define NUM_THREADS 3
#define NUM_TASKS	10

struct task_t {
	void (*routine_for_task)(void *);
	void *arg;
	struct task_t *next;
};

struct threadpool {
    pthread_mutex_t		mutex;
    pthread_t			*threads;
	pthread_cond_t		cond;
	int					counter;
    struct task_t		*tasks;
};

extern struct threadpool* Thpool_create();

extern int Thpool_add_task(struct threadpool *pool,
							struct task_t job);

void * Thpool_routine(void *threadpool);

extern int Thpool_destroy(struct threadpool *pool);

extern int Thpool_free(struct threadpool *pool);

#endif /* _THPOOL_H*/

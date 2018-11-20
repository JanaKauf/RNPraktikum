
#ifndef _TASKQUEUE_H
#define _TASKQUEUE_H
#include <mqueue.h>
#include <stdbool.h>

struct task {
   void (*routine_for_task)(void *arg); 
   void *arg;	
};

extern mqd_t taskqueue_create(const char * name,
		const unsigned int size);

extern void taskqueue_close(const mqd_t mqdes);

extern void taskqueue_destroy(const char * name);

extern int taskqueue_send(const mqd_t mqdes,
		const struct task t,
		const unsigned int prio,
		bool blocking);

extern struct task taskqueue_receive(const mqd_t mqdes);

#endif    /*_TASKQUEUE_H */

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include "../util/so_scheduler.h"
#include <stdbool.h>

typedef struct Thread{
	pthread_t tid;
	unsigned int priority;
	so_handler *routine;
    int time;

	sem_t sem;
}*thread;

#define MAX_THREADS 100


// sorts descending  by priority
typedef struct Priority_queue{
    thread arr[MAX_THREADS];
    int len;
	int start;
}*pq;


typedef struct Scheduler{
	int quantum;
	int max_io;

	thread running_thread;

	int no_ready_threads;
	pq ready_threads;

	int no_blocked_threads;
	int blocked_threads[MAX_THREADS];

	int no_terminated_threads;
	int terminated_threads[MAX_THREADS];


}scheduler;


static scheduler sched;


pq init_pq();
bool is_empty(pq x);
void delete_pq(pq x);
void push(pq x, thread t);
thread init_thread(unsigned int priority,int time);
void *start_thread(void *args);
void free_thread(thread th);
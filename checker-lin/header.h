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
	unsigned int waiting_for_io;
	// 0 - new 1- ready 2- waiting/ blocked 3 - running  4 - terminated
	int state; 
	sem_t sem;
}*thread;

#define MAX_THREADS 100

typedef struct node {
    thread data;


   unsigned  int priority;
 
    struct node* next;
 
} Node;
 

typedef struct Scheduler{
	int quantum;
	int max_io;

	thread running_thread;

	int no_ready_threads;
	Node *ready_threads;

	int no_blocked_threads;
	thread blocked_threads[MAX_THREADS];

	int no_terminated_threads;
	thread terminated_threads[MAX_THREADS];

	sem_t ending_sem;
}scheduler;



Node* newNode(thread d,unsigned int p);
thread peek(Node** head);
void pop(Node** head);
void push(Node** head, thread d,unsigned int p);
bool  isEmpty(Node** head);
void deleteList(Node **head);



thread init_thread(unsigned int priority,int time,so_handler *func);
void *start_thread(void *params);
void free_thread(thread th);
void schedule();
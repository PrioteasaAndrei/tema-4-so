#include "header.h"


thread init_thread(unsigned int priority,int time,so_handler *func){
    thread t = (thread)malloc(sizeof(struct Thread *));
    t->priority = priority;
    t->time = time;
    t->routine = func;
    t->waiting_for_io = -1;
    t->state = 0;
    sem_init(&t->sem,0,0);
    return t;

    // asta trebuie apelata dupa init
    // pthread_create(&t->tid,NULL,&start_thread,(void*)t);
}

void free_thread(thread th){
    sem_destroy(&th->sem);
    free(th);
}

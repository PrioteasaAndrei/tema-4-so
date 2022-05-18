#include "priority_queue.h"


pq init_pq(){
    pq my_pq = (pq)malloc(sizeof(struct Priority_queue *));
    my_pq->len = 0;
    my_pq->start = 0;
    return my_pq;
}

bool is_empty(pq x){
    return (x->len == 0);
}


void delete_pq(pq x){

    // free the threads
    // TODO : free the interior of the struct of threads
    for(int i=0; i< x->len; ++i){
        free(x->arr[i]);
    }
    free(x);
}
// terrible design but spares me a lot of time
// maybe fix it at the end
// the thread will still be there but
// i will access it from somewhere else
thread pop(pq x){
    if(is_empty(x)){
        return NULL;
    }

    if(x->start == x->len){
        return NULL;
    }

    thread tbr = x->arr[x->start];
    x->start ++;
    return tbr;

}

void push(pq x, thread t){

    if(sched.no_ready_threads + 1 > MAX_THREADS){
        return;
    }

    if(x->len == 0){
        x->arr[0] = t;
        x->len ++;
        return;
    }

    int index = -1;
    for(int i=0;i<x->len; ++i){
        if(t->priority >= x->arr[i]->priority){
            index = i;
            break;
        }
    }

    if(index == -1){
        // insert at the end
        x->arr[x->len] = t;
        x->len ++;
    }else{
        for(int i = x->len - 1; i >= index; --i){
            x->arr[i +1] = x->arr[i];
        }

        x->arr[index] = t;
        x->len ++;
    }


}


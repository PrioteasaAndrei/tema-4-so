
#include "header.h"

static scheduler sched;


int so_init(unsigned int time_quantum, unsigned int io){
	
	if(io > SO_MAX_NUM_EVENTS){
		return -1;
	}

	

	sched.max_io = io;
	sched.quantum = time_quantum;
	sched.no_ready_threads = 0;
	sched.running_thread = NULL;
	sched.no_blocked_threads = 0;
	sched.no_terminated_threads = 0;

	// init sem
	sem_init(&sched.ending_sem,0,1);
	return 0;
}


tid_t so_fork(so_handler *func, unsigned int priority){
	// alocate and initialize new thread
	thread th = init_thread(priority,sched.quantum,func);
	th->routine = func;
	th->state = 1;
	pthread_create(&(th->tid),NULL,&start_thread,(void*)th);

	// el da aici thread_routine
	// pthread create da si start la thread cu rutina aia

	
	// trebuie sa deosebeasc cumva threadurile
	// ready de cele blocked
	

	// add thread to ready state queue

	// thread state ready

	if(isEmpty(&sched.ready_threads)){
		sched.ready_threads = newNode(th,priority);
	}else{
		push(&sched.ready_threads,th,priority);
	}

	// need to pass time so call so_exec()

	if(sched.running_thread != NULL){
		so_exec();
	}else{
		schedule();
	}

	return th->tid;

}


 // a step of the scheduler

void schedule(){


	// only the running thread remaining
	if(isEmpty(&sched.ready_threads)){
		
		if(sched.running_thread != NULL){
			if(sched.running_thread->state == 4){
				// done 
				// end scheduler
				sem_post(&sched.ending_sem);
			}else{
				// still execs to do
				sem_post(&sched.running_thread->sem);			
			}
		}else{
			// end scheduler
			// nu o sa intre niciodata aici dar nu conteaza
			sem_post(&sched.ending_sem);
		}
		
		return;
	}



	thread max_prio_thread = peek(&sched.ready_threads);


	// time to run another thread
	if(sched.running_thread == NULL){
		
		if(isEmpty(&sched.ready_threads)){
			// should'nt be here
			printf("ERROR\n");
			return;
		}

		
		// state is running now
		max_prio_thread->state = 3;
		max_prio_thread->time = sched.quantum;
		sched.running_thread = max_prio_thread;
		pop(&sched.ready_threads);

		sem_post(&max_prio_thread->sem);
	}else{

		// terminated
		if(sched.running_thread->state == 4){
			sched.terminated_threads[sched.no_terminated_threads] = sched.running_thread;
			sched.no_terminated_threads++ ;
			pop(&sched.ready_threads);

			max_prio_thread->state = 3;
			max_prio_thread->time = sched.quantum;
			sched.running_thread = max_prio_thread;

			
			sem_post(&sched.running_thread->sem);
			return;
		}

		// waiting
		if(sched.running_thread->state == 2){
			sched.blocked_threads[sched.no_blocked_threads] = sched.running_thread;
			sched.no_blocked_threads ++ ;
			max_prio_thread->state = 3;
			max_prio_thread->time = sched.quantum;
			sched.running_thread = max_prio_thread;
			pop(&sched.ready_threads);

			sem_post(&sched.running_thread->sem);
			return;
		}


		// preemption by higher priority thread
		// = pt ca vreau sa il lase pe altul cu aceiasi prioritate
		// problema va fi ca va cicla doar intre el si precedentul
		// dar merge deocamdata
		if(max_prio_thread->priority >= sched.running_thread->priority){
			pop(&sched.ready_threads);
			push(&sched.ready_threads,sched.running_thread,sched.running_thread->priority);
			max_prio_thread->state = 3;
			max_prio_thread->time = sched.quantum;
			sched.running_thread = max_prio_thread;

			sem_post(&sched.running_thread->sem);
			return;
		}

		// expired quantum
		if(sched.running_thread->time <= 0){
			// round robin shit
			sched.running_thread->time = sched.quantum;
			sem_post(&sched.running_thread->sem);
			return;
		}

	}
}

// so exec because wait and signal are considered instructions
int so_wait(unsigned int io){
	// blocked
	sched.running_thread->state = 2;
	sched.running_thread->waiting_for_io = io;
	// asta apeleaza schedule() care vede ca e blocked 
	// si il scoate
	so_exec();
	return 0;
}


int so_signal(unsigned int io){
	int n = sched.no_blocked_threads;
	int tbr = 0;
	for(int i=0;i<n; ++i){
		thread cur = sched.blocked_threads[i];

		// these are threads that have been
		// signaled but i didn't remove them
		// from the blocked vector
		// i just changed their state
		// if(cur->state != 2){
		// 	printf("Error in so_signal\n");
		// 	return -1;
		// }

		if(cur->state == 2 &&  cur->waiting_for_io == io){
			cur->waiting_for_io = -1;
			cur->state = 1;

			// put it back in the ready queue

			push(&sched.ready_threads,cur,cur->priority);
			tbr++;
		}
	}
	so_exec();
	return tbr;
}

void so_exec(void){
	sched.running_thread->time -- ;
	schedule();
	sem_wait(&sched.running_thread->sem);
}


void so_end(void){
	// wait

	sem_wait(&sched.ending_sem);
	int n = sched.no_terminated_threads;
	for(int i=0;i<n; ++i){
		thread cur = sched.terminated_threads[i];

		pthread_join(cur->tid,NULL);
	}


	// incearca asta si sus
	for(int i=0; i<n; ++i){
		thread cur = sched.terminated_threads[i];
		free_thread(cur);
	}

	sem_destroy(&sched.ending_sem);

}


// static ? 
// void * pt argument pthread_create
void* start_thread(void *params){
	// incepe instant sa ruleze de aici
	thread th = (thread)params;

	// wait to be planned
	// se blocheaza aici pana ii da cineva sem_post
	sem_wait(&th->sem);

	th->routine(th->priority);

	// terminated move to terminated list

	th->state = 4;
	schedule();

	return NULL;
}
#define INTERVAL 500
#define JB_SP 4
#define JB_PC 5

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "thread_manager.hpp"

static int ptr_mangle(int p) {
    unsigned int ret;
    asm(" movl %1, %%eax;\n"
        " xorl %%gs:0x18, %%eax;"
        " roll $0x9, %%eax;"
        " movl %%eax, %0;"
    : "=r"(ret)
    : "r"(p)
    : "%eax"
    );
    return ret;
}

void alarm_handler(int signo) {
	if (setjmp(ThreadManager::get().getRunningTCB()->buf) == 0) {
		ThreadManager::get().nextThread();
	}
}

void initTimer() {
	struct itimerval it_val;
	struct sigaction act;
	act.sa_handler = alarm_handler;
	act.sa_flags = SA_NODEFER;
	if(sigaction(SIGALRM, &act, NULL) == -1) {
		perror("Unable to catch SIGALRM");
		exit(1);
	}

	it_val.it_value.tv_sec = INTERVAL/1000;
	it_val.it_value.tv_usec = (INTERVAL*1000) % 1000000;
	it_val.it_interval = it_val.it_value;

	if(setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
		perror("error calling setitimer()");
		exit(1);
	}
}

ThreadManager::ThreadManager()
: highest_thread_id{0}
, mapTCB()
, running_thread_it() {
	TCB* curr_tcb = new TCB(highest_thread_id); /* main thread receives id 0 */
	setjmp(curr_tcb->buf);
	this->mapTCB[curr_tcb->thread_id] = curr_tcb;
	running_thread_it = this->mapTCB.begin();
	initTimer();
}

ThreadManager::~ThreadManager() {
	for (auto it = this->mapTCB.begin(); it != this->mapTCB.end(); it++) {
		delete it->second;
	}
}

void ThreadManager::createThread(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void *), void *arg, void (*exit_addr)(void *)) {
	*thread = ++(this->highest_thread_id);
	TCB* crt_tcb = new TCB(*thread);

	setjmp(crt_tcb->buf);

	uint8_t* stack_ptr = ((uint8_t *) (crt_tcb->stack_top));

	*((int *) (stack_ptr + STACK_SIZE - 1 * sizeof(int))) = (intptr_t) arg;
	*((int *) (stack_ptr + STACK_SIZE - 2 * sizeof(int))) = (intptr_t) exit_addr;

	crt_tcb->buf->__jmpbuf[JB_SP] = ptr_mangle((intptr_t)(stack_ptr + STACK_SIZE - 2 * sizeof(int)));
	crt_tcb->buf->__jmpbuf[JB_PC] = ptr_mangle((intptr_t)(start_routine));

	this->mapTCB[crt_tcb->thread_id] = crt_tcb;
}

[[ noreturn ]] void ThreadManager::finishCurrentThread() {
	pthread_t curr_thread_id = this->running_thread_it->second->thread_id;
	TCB* fin_tcb = this->mapTCB.find(curr_thread_id)->second;
	delete fin_tcb;
	this->running_thread_it = this->mapTCB.erase(this->running_thread_it);
	if (this->running_thread_it == this->mapTCB.end())
		this->running_thread_it = this->mapTCB.begin();
	if (this->mapTCB.size() == 0)
		exit(0);
	longjmp(this->running_thread_it->second->buf, 1);
}

TCB* ThreadManager::getRunningTCB() { return this->mapTCB[this->running_thread_it->second->thread_id]; }

/* Invariant is that the running_thread_it is an iterator to a valid value */
[[ noreturn ]] void ThreadManager::nextThread() {
	assert(this->mapTCB.size() > 0);
	this->running_thread_it++;
	if (this->running_thread_it == this->mapTCB.end())
		this->running_thread_it = this->mapTCB.begin();
	longjmp(this->running_thread_it->second->buf, 1);
} 

pthread_t pthread_self() {
	return ThreadManager::get().getRunningTCB()->thread_id;
}

void pthread_exit(void* value_ptr) {
	ThreadManager::get().finishCurrentThread();
}

int pthread_create(pthread_t *restrict_thread, const pthread_attr_t *restrict_attr, void *(*start_routine)(void*), void *restrict_arg) {
	ThreadManager::get().createThread(restrict_thread, NULL, start_routine, restrict_arg, *pthread_exit);
	return 0;
}

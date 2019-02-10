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
	if (setjmp(ThreadManager::get().getRunningTCB().buf) == 0) {
		ThreadManager::get().getRunningTCB().state = READY;
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
: num_threads{0}
, curr_thread_i{0} {
	TCB curr_tcb(num_threads); /* main thread receives id 0 */
	curr_tcb.state = RUNNING;
	setjmp(curr_tcb.buf);
	this->tcb_arr[num_threads] = curr_tcb;
	num_threads++;
	initTimer();
}

void ThreadManager::createThread(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void *), void *arg, void (*exit_addr)(void *)) {
	*thread = this->num_threads;
	TCB crt_tcb(*thread);
	setjmp(crt_tcb.buf);

	uint8_t* stack_ptr = ((uint8_t *) (crt_tcb.stack_top));

	*((int *) (stack_ptr + STACK_SIZE - 1 * sizeof(int))) = (intptr_t) arg;
	*((int *) (stack_ptr + STACK_SIZE - 2 * sizeof(int))) = (intptr_t) exit_addr;

	crt_tcb.buf->__jmpbuf[JB_SP] = ptr_mangle((intptr_t)(stack_ptr + STACK_SIZE - 2 * sizeof(int)));
	crt_tcb.buf->__jmpbuf[JB_PC] = ptr_mangle((intptr_t)(start_routine));
	crt_tcb.state = READY;

	this->tcb_arr[num_threads] = crt_tcb;
	num_threads++;
}

[[ noreturn ]] void ThreadManager::finishCurrentThread() {
	TCB fin_tcb = this->tcb_arr[curr_thread_i];
	free(fin_tcb.stack_top);
	fin_tcb.state = TERMINATED;
	this->nextThread();
}

TCB& ThreadManager::getRunningTCB() { return this->tcb_arr[curr_thread_i]; }

[[ noreturn ]] void ThreadManager::nextThread() {
	int i;
	for (i = 0; i < this->num_threads; i++) {
		this->curr_thread_i = (this->curr_thread_i + 1) % this->num_threads;
		if (this->tcb_arr[curr_thread_i].state == READY)
			break;
	}
	if (i == this->num_threads)
		exit(0);

	this->tcb_arr[curr_thread_i].state = RUNNING;
	longjmp(this->tcb_arr[curr_thread_i].buf, 1);
} 

pthread_t pthread_self() {
	return ThreadManager::get().getRunningTCB().thread_id;
}

void pthread_exit(void* value_ptr) {
	ThreadManager::get().finishCurrentThread();
}

int pthread_create(pthread_t *restrict_thread, const pthread_attr_t *restrict_attr, void *(*start_routine)(void*), void *restrict_arg) {
	ThreadManager::get().createThread(restrict_thread, NULL, start_routine, restrict_arg, *pthread_exit);
	return 0;
}

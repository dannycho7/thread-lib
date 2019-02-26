#define INTERVAL 500
#define JB_SP 4
#define JB_PC 5
#define MAIN_THREAD 0

#include <cstdint>
#include <iostream>
#include <semaphore.h>
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

static void _modALRM(int how) {
	sigset_t alrm_set;
	sigemptyset(&alrm_set);
	sigaddset(&alrm_set, SIGALRM);
	sigprocmask(how, &alrm_set, NULL);
}

void lock() {
	_modALRM(SIG_BLOCK);
}

void unlock() {
	_modALRM(SIG_UNBLOCK);
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
	sigemptyset(&act.sa_mask);
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

void ThreadManager::createThread(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void *), void *arg, void (*exit_addr)()) {
	*thread = this->num_threads;
	TCB crt_tcb(*thread);
	setjmp(crt_tcb.buf);
	uint8_t* stack_ptr = ((uint8_t *) (crt_tcb.stack_top));
	*((int *) (stack_ptr + STACK_SIZE - 1 * sizeof(int))) = (int) arg;
	*((int *) (stack_ptr + STACK_SIZE - 2 * sizeof(int))) = (int) exit_addr;
	crt_tcb.buf->__jmpbuf[JB_SP] = ptr_mangle((intptr_t)(stack_ptr + STACK_SIZE - 2 * sizeof(int)));
	crt_tcb.buf->__jmpbuf[JB_PC] = ptr_mangle((intptr_t)(start_routine));
	crt_tcb.state = READY;

	this->tcb_arr[num_threads] = crt_tcb;
	num_threads++;
}

[[ noreturn ]] void ThreadManager::finishCurrentThread(void* value_ptr) {
	lock();
	TCB& fin_tcb = this->tcb_arr[this->curr_thread_i];
	fin_tcb.return_val = value_ptr;
	if (fin_tcb.waiting_thread != NULL) {
		fin_tcb.waiting_thread->state = READY;
	}
	fin_tcb.state = TERMINATED;
	unlock();
	this->nextThread();
}

TCB& ThreadManager::getRunningTCB() { return this->tcb_arr[curr_thread_i]; }
TCB& ThreadManager::getTCBByThreadId(pthread_t thread_id) {
	for (int i = 0; i < this->num_threads; i++) {
		if (this->tcb_arr[i].thread_id == thread_id) {
			return this->tcb_arr[i];
		}
	}
	throw std::runtime_error("Invalid thread.");
}

[[ noreturn ]] void ThreadManager::nextThread() {
	int i;
	for (i = 0; i < this->num_threads; i++) {
		this->curr_thread_i = (this->curr_thread_i + 1) % this->num_threads;
		if (this->tcb_arr[curr_thread_i].state == READY)
			break;
	}
	if (i == this->num_threads) {
		for (int i = 1; i < this->num_threads; i++) {
			if (i == this->curr_thread_i) continue; // don't free current thread's stack
			free(this->tcb_arr[i].stack_top);
		}
		exit(0);
	}
	this->tcb_arr[curr_thread_i].state = RUNNING;
	longjmp(this->tcb_arr[curr_thread_i].buf, 1);
} 

pthread_t pthread_self() {
	return ThreadManager::get().getRunningTCB().thread_id;
}

void pthread_exit(void* value_ptr) {
	ThreadManager::get().finishCurrentThread(value_ptr);
}

void pthread_exit_wrapper() {
	unsigned int res;
	asm("movl %%eax, %0\n":"=r"(res));
	pthread_exit((void *) res);
}

int pthread_create(pthread_t *restrict_thread, const pthread_attr_t *restrict_attr, void *(*start_routine)(void*), void *restrict_arg) {
	ThreadManager::get().createThread(restrict_thread, NULL, start_routine, restrict_arg, pthread_exit_wrapper);
	return 0;
}

int pthread_join(pthread_t thread, void **value_ptr) {
	lock();
	TCB& thread_data = ThreadManager::get().getTCBByThreadId(thread);
	if (thread_data.state == TERMINATED) {
		unlock();
	} else {
		thread_data.waiting_thread = &(ThreadManager::get().getRunningTCB());
		ThreadManager::get().getRunningTCB().state = BLOCKED;
		unlock();
		if (setjmp(ThreadManager::get().getRunningTCB().buf) == 0)
			ThreadManager::get().nextThread();
	}
	*value_ptr = thread_data.return_val;

	return 0;
}

int sem_init(sem_t *sem, int pshared, unsigned value) {
	__sem_t* add_info = new __sem_t(value);
	sem->__align = (long int) add_info;
	return 0;
}

int sem_destroy(sem_t *sem) {
	delete ((__sem_t *) sem->__align);
	return 0;
}
int sem_wait(sem_t *sem) {
	lock();
	__sem_t* add_info = ((__sem_t *) sem->__align);
	if (add_info->value == 0) {
		add_info->waiting_threads.push_back(&ThreadManager::get().getRunningTCB());
		ThreadManager::get().getRunningTCB().state = BLOCKED;
		unlock();
		if (setjmp(ThreadManager::get().getRunningTCB().buf) == 0)
			ThreadManager::get().nextThread();
	} else {
		add_info->value--;
		unlock();
	}
	return 0;
}
int sem_post(sem_t *sem) {
	lock();
	__sem_t* add_info = ((__sem_t *) sem->__align);
	if (add_info->value == 0 && add_info->waiting_threads.size() > 0) {
		TCB* next_owner = add_info->waiting_threads.front();
		add_info->waiting_threads.pop_front();
		next_owner->state = READY;
		unlock();
	} else {
		add_info->value++;
		unlock();
	}
	return 0;
}

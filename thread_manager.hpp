#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H
#define STACK_SIZE 32767
#define MAX_THREADS 128

#include <map>
#include <pthread.h>
#include <setjmp.h>

enum TCB_STATE { RUNNING, READY, TERMINATED };

struct TCB {
	pthread_t thread_id;
	int* stack_top;
	jmp_buf buf;
	TCB_STATE state;
	TCB() {}
	TCB(pthread_t thread_id) {
		this->thread_id = thread_id;
		int* stack = (int *) malloc(STACK_SIZE);
		this->stack_top = stack;
	}
};

class ThreadManager {
public:
	static ThreadManager& get() {
		static ThreadManager tm;
		return tm;
	}
	void createThread(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void *), void *arg, void (*exit_addr)(void *));
	[[ noreturn ]] void finishCurrentThread();
	TCB& getRunningTCB();
	[[ noreturn ]] void nextThread();
private:
	ThreadManager();
	int num_threads, curr_thread_i;
	TCB tcb_arr[MAX_THREADS];
};
#endif

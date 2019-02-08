#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H
#define STACK_SIZE 32767

#include <map>
#include <pthread.h>
#include <setjmp.h>

struct TCB {
	pthread_t thread_id;
	int* stack_top;
	jmp_buf buf;
	TCB(pthread_t thread_id) {
		this->thread_id = thread_id;
		int *stack = (int *) malloc(STACK_SIZE);
		this->stack_top = stack;
	}
	~TCB() {
		free(this->stack_top);
	}
};

class ThreadManager {
public:
	static ThreadManager get() {
		static ThreadManager tm;
		return tm;
	}
	void finishTCB(pthread_t id);
	void createTCB(pthread_t* t, pthread_attr_t* attr, void* (*start_routine)(void *), void *arg, void* (*exit_addr));
	TCB* getRunningTCB();
private:
	ThreadManager()
	: running_thread_id{0}
	, highest_thread_id{0}
	, TCBs() {
		TCB* curr_tcb = new TCB(++highest_thread_id);
		setjmp(curr_tcb->buf);
		this->TCBs[curr_tcb->thread_id] = curr_tcb;
	}

	pthread_t running_thread_id;
	pthread_t highest_thread_id;
	std::map<pthread_t, TCB*> TCBs;
};
#endif

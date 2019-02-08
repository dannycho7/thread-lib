#define JB_SP 4
#define JB_PC 5

#include <cstdint>
#include <stdlib.h>
#include "thread_manager.h"

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

void ThreadManager::createThread(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void *), void *arg, void (*exit_addr)(void *)) {
	*thread = ++(this->highest_thread_id);
	TCB* crt_tcb = new TCB(*thread);

	setjmp(crt_tcb->buf);

	crt_tcb->stack_top[STACK_SIZE - 1] = (intptr_t) arg;
	crt_tcb->stack_top[STACK_SIZE - 2] = (intptr_t) exit_addr;

	crt_tcb->buf->__jmpbuf[JB_SP] = ptr_mangle((intptr_t)(crt_tcb->stack_top + STACK_SIZE - 2));
	crt_tcb->buf->__jmpbuf[JB_PC] = ptr_mangle((intptr_t)(start_routine));

	this->TCBs[crt_tcb->thread_id] = crt_tcb;
}

[[ noreturn ]] void ThreadManager::finishCurrentThread() {
	pthread_t curr_thread_id = this->running_thread_it->second->thread_id;
	TCB* fin_tcb = this->TCBs.find(curr_thread_id)->second;
	delete fin_tcb;
	this->TCBs.erase(curr_thread_id);
	ThreadManager::get().nextThread();
}

TCB* ThreadManager::getRunningTCB() { return this->TCBs[this->running_thread_it->second->thread_id]; }

[[ noreturn ]] void ThreadManager::nextThread() {
	if (this->TCBs.size() == 0)
		exit(0);
	else {
		if (this->running_thread_it == this->TCBs.end())
			this->running_thread_it = this->TCBs.begin();
		else if (this->TCBs.size() > 1)
			this->running_thread_it++;
	}
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

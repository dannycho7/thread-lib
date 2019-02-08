/* Maybe we don't need these */
#define JB_BX 0
#define JB_SI 1
#define JB_DI 2
#define JB_BP 3
#define JB_SP 4
#define JB_PC 5
/* Potentially remove */

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

void ThreadManager::finishTCB(pthread_t thread_id) {
	TCB* fin_tcb = this->TCBs.find(thread_id)->second;
	delete fin_tcb;
	this->TCBs.erase(thread_id);
}

void ThreadManager::createTCB(pthread_t* thread, pthread_attr_t* attr, void* (*start_routine)(void *), void *arg, void* (*exit_addr)) {
	*thread = ++(this->highest_thread_id);
	TCB* crt_tcb = new TCB(*thread);

	setjmp(crt_tcb->buf);

	crt_tcb->stack_top[STACK_SIZE - 1] = (intptr_t) arg;
	crt_tcb->stack_top[STACK_SIZE - 2] = (intptr_t) exit_addr;

	crt_tcb->buf->__jmpbuf[JB_SP] = ptr_mangle((intptr_t)(crt_tcb->stack_top + STACK_SIZE - 1));
	crt_tcb->buf->__jmpbuf[JB_PC] = ptr_mangle((intptr_t)(start_routine));

	this->TCBs[crt_tcb->thread_id] = crt_tcb;
}

TCB* ThreadManager::getRunningTCB() { return this->TCBs[this->running_thread_id]; }

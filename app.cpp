#include <iostream>
#include "thread_manager.h"

int main() {
	std::cout << "Before get" << std::endl;
	if (setjmp(ThreadManager::get().getRunningTCB()->buf) == 0)
		ThreadManager::get().nextThread();
	std::cout << "Out of get" << std::endl;
}
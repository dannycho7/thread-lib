#include <iostream>
#include "thread_manager.hpp"

int main() {
	std::cout << "Before get" << std::endl;
	ThreadManager::get();
	std::cout << "Out of get" << std::endl;
	while(1);
}
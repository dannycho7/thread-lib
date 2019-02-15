# C++ Thread Library

This thread library consists of a Singleton class named `ThreadManager`. `ThreadManager` handles all low level details of creating, switching, and deleting threads. A SIGALRM signal would trigger as soon as the singleton is first accessed and essentially perform a `setjmp`-`longjmp` combo. The actual interface was exposed via the `pthread_create`, `pthread_self`, and `pthread_exit` functions. These functions called into the `ThreadManager` Singleton to perform the actions.

Compiled with g++ on CSIL.
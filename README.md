# C++ Thread Library

This thread library consists of a Singleton class named `ThreadManager`. `ThreadManager` handles all low level details of creating, switching, and deleting threads. A SIGALRM signal would trigger as soon as the singleton is first accessed and essentially perform a `setjmp`-`longjmp` combo. The actual interface was exposed via the `pthread_create`, `pthread_self`, and `pthread_exit` functions. These functions called into the `ThreadManager` Singleton to perform the actions.

In order to implement locking, I disabled/enabled the alarm signal. Additional semaphore metadata was stored using the newly created `__sem_t` struct stored in the `sem_t::align` as a `long int`. When threads would wait via `pthread_join` or `sem_wait`, the `ThreadManager` would modify the TCB to be in a blocked state, which prevents the threads from being scheduled.

Compiled with g++ on CSIL.
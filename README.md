# C++ Thread Library

Created By: Hyun Bum Cho (4195368)

In order to create this thread library, I created a Singleton class named `ThreadManager`. `ThreadManager` would handle all the low level details of creating, switching, and deleting threads. The alarm would trigger as soon as the singleton is first accessed and essentially perform a `setjmp`-`longjmp` combo. The actual interface was exposed via the `pthread_create`, `pthread_self`, and `pthread_exit` functions. These functions called into the `ThreadManager` Singleton to perform the actions.

Compiled with g++ on CSIL.
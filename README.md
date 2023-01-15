# MockThreads
Unlimited virtual user threads mapped to four virtual kernel threads. This is accomplished using C++ and Inline Assembly.     
Threads can be scheduled using FIFO or through Priority Scheduling. The scheduler can interrupt, schedule, and reschedule threads on any of the four kernel threads, so they each have their own stack pointer, stack base pointer, and registers     
If you would like to test it, just run ```make && ./uthread 7```

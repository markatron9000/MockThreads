#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "uthread.h"

void foo (void* arg) {
    
    for (size_t i = 0; i < 10; ++i) {
        printf("Thread %lu\n", (unsigned long)arg);
        usleep(1000);
    }
    printf("Thread %lu done.\n", (unsigned long)arg);
    uthread_exit();
}

void barV(void* arg) {
    uthread_set_param((unsigned long)arg);
    printf("First time thread %lu running\n", (unsigned long)arg);
    for (size_t i = 0; i < 10; ++i) {
        usleep(1000);
        printf("i = ");
        printf("%lu", i);
        printf(", ");
        printf("Thread = %lu\n", (unsigned long)arg);
        uthread_yield();
    }
    printf("Thread %lu done.\n", (unsigned long)arg);
    uthread_exit();
}

void bar(void* arg) {
    uthread_set_param((unsigned long)arg);
    for (size_t i = 0; i < 10; ++i) {
        usleep(1000);
        uthread_yield();
    }
    printf("Thread %lu done.\n", (unsigned long)arg);
    uthread_exit();
}


int main(int argc, const char** argv){
    int test = -1;
    if (argc > 1) {
        test = atoi(argv[1]);
    }

    switch(test) {
        case 1:
            puts("\e[34m# Test 1: Task 1.\e[0m");
            uthread_set_policy(UTHREAD_DIRECT_PTHREAD);
            uthread_init();
            uthread_create(foo, (void*)1);
            uthread_create(foo, (void*)2);
            uthread_cleanup();
            break;
        case 2:
            puts("\e[34m# Test 2: Task 2, fewer uthreads than pthreads.\e[0m");
            uthread_set_policy(UTHREAD_PRIORITY);
            uthread_init();
            uthread_create(foo, (void*)1);
            uthread_create(foo, (void*)2);
            uthread_cleanup();
            break;
       case 3:
            puts("\e[34m# Test 3: Task 2, equal number of uthreads and pthreads.\e[0m");
            uthread_set_policy(UTHREAD_PRIORITY);
            uthread_init();
            for (size_t i = 0; i < 4; i++) {
                uthread_create(foo, (void*)i);
            }
            uthread_cleanup();
            break;
        case 4:
            puts("\e[34m# Test 4: Task 2, Testing more uthreads than pthreads.\e[0m");
            setVerbose(true);
            uthread_set_policy(UTHREAD_PRIORITY);
            uthread_init();
            for (size_t i = 0; i < 6; i++) {
                uthread_create(foo, (void*)i);
            }
            uthread_cleanup();
            break;
        case 5:
            puts("\e[34m# Test 5: Task 2, trying priority stuff and yielding. Expect threads 0-3 to finish first then threads 4-7.\e[0m");
            uthread_set_policy(UTHREAD_PRIORITY);
            uthread_init();
            for (size_t i = 0; i < 8; i++) {
                uthread_create(bar, (void*)(i));
            }
            uthread_cleanup();
            break;
        case 6:
            puts("\e[34m# Test 6: Task 2, trying priority stuff and yielding with extra logging. Expect threads 0-3 to finish first then threads 4-7.\e[0m");
            setVerbose(true);
            uthread_set_policy(UTHREAD_PRIORITY);
            uthread_init();
            for (size_t i = 0; i < 8; i++) {
                uthread_create(bar, (void*)i);
            }
            uthread_cleanup();
            break;
        case 7:
            puts("\e[34m# Test 7: Task 2, trying priority stuff and yielding with extra verbosity. Expect threads 0-3 to finish first then threads 4-7.\e[0m");
            setVerbose(true);
            uthread_set_policy(UTHREAD_PRIORITY);
            uthread_init();
            for (size_t i = 0; i < 8; i++) {
                uthread_create(barV, (void*)i);
            }
            uthread_cleanup();
            break;
        case 8:
            puts("\e[34m# Test 1: Task 1. Verbose.\e[0m");
            uthread_set_policy(UTHREAD_DIRECT_PTHREAD);
            setVerbose(true);
            uthread_init();
            uthread_create(foo, (void*)1);
            uthread_create(foo, (void*)2);
            uthread_cleanup();
            break;
        default:
            puts("Invalid test number");
            break;
    }
    return 0;
}
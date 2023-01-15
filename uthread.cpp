#include <pthread.h>
#include <signal.h>
#include "structs.h"
#include "uthread.h"
#include <iostream>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <map>
#include <list>
#include <functional>
#include <sys/epoll.h>
#include <algorithm>
#include <map>
#include "threadSafeQueue.cpp"
#include "threadSafePriorityQueue.cpp"

// An atomic boolean so that we can tell our kernel threads we want them to exit when finshed with current task, if they are working on something
std::atomic_bool whenFinshedJustStop{false};

// A bool that will let us know if we are to use priority based scheduling or not
bool usePriorityQueue = false;

// A switch for turning on verbose messaging
bool verbosey = false;

// Our 4 kernel threads
kernelThreadObject kernelThread1;
kernelThreadObject kernelThread2;
kernelThreadObject kernelThread3;
kernelThreadObject kernelThread4;

// A thread safe queue for our virtual threads
ThreadsafeQueue threadWaitingQueue;

// A thread safe priority queue for our virtual threads (used if schedulig policy set to be priority)
ThreadsafePriorityQueue priorityThreadWaitingQueue;

// Holds the current task being run
__thread threadObject* current_uthread;

// Keeps track of how many kernel threads have finished
int kernelThreadsFinished = 0;

// Keeps track of how many virtual "user threads" have been created by the user
int totalUserThreadsCreated = 0;

// Save all x86_64 registers
void saveAllRegisters(processRegisters* saveTo){
    __asm__ __volatile__("movq %%r8, %0" : "=r"(saveTo->r8): : "r8");
    __asm__ __volatile__("movq %%r9, %0" : "=r"(saveTo->r9): : "r9");
    __asm__ __volatile__("movq %%r10, %0" : "=r"(saveTo->r10): : "r10");
    __asm__ __volatile__("movq %%r11, %0" : "=r"(saveTo->r11): : "r11");
    __asm__ __volatile__("movq %%r12, %0" : "=r"(saveTo->r12): : "r12");
    __asm__ __volatile__("movq %%r13, %0" : "=r"(saveTo->r13): : "r13");
    __asm__ __volatile__("movq %%r14, %0" : "=r"(saveTo->r14): : "r14");
    __asm__ __volatile__("movq %%r15, %0" : "=r"(saveTo->r15): : "r15");
    __asm__ __volatile__("movq %%rax, %0" : "=r"(saveTo->rax): : "rax"); 
    __asm__ __volatile__("movq %%rbx, %0" : "=m"(saveTo->rbx): : "rbx");
    __asm__ __volatile__("movq %%rcx, %0" : "=r"(saveTo->rcx): : "rcx");
    __asm__ __volatile__("movq %%rdx, %0" : "=r"(saveTo->rdx): : "rdx");
    __asm__ __volatile__("movq %%rsi, %0" : "=r"(saveTo->rsi): : "rsi");
    __asm__ __volatile__("movq %%rdi, %0" : "=r"(saveTo->rdi): : "rdi");
    __asm__ __volatile__("movq %%rsp, %0" : "=g"(saveTo->rsp): : "rsp");
    __asm__ __volatile__("movq %%rbp, %0" : "=m"(saveTo->rbp): : "rax", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11","memory", "cc");
}

// Sett all x86_64 registers, and jump to stored place
void setAllRegisters(processRegisters* setFrom){
    __asm__ __volatile__("movq %0, %%r8" : : "r"(setFrom->r8): "r8");
    __asm__ __volatile__("movq %0, %%r9" : : "r"(setFrom->r9): "r9");
    __asm__ __volatile__("movq %0, %%r10" : : "r"(setFrom->r10): "r10");
    __asm__ __volatile__("movq %0, %%r11" : : "r"(setFrom->r11): "r11");
    __asm__ __volatile__("movq %0, %%r12" : : "r"(setFrom->r12): "r12");
    __asm__ __volatile__("movq %0, %%r13" : : "r"(setFrom->r13): "r13");
    __asm__ __volatile__("movq %0, %%r14" : : "r"(setFrom->r14): "r14");
    __asm__ __volatile__("movq %0, %%r15" : : "r"(setFrom->r15): "r15");
    __asm__ __volatile__("movq %0, %%rax" : : "r"(setFrom->rax): "rax");
    __asm__ __volatile__("movq %0, %%rbx" : : "m"(setFrom->rbx): "rbx");
    __asm__ __volatile__("movq %0, %%rcx" : : "r"(setFrom->rcx): "rcx");
    __asm__ __volatile__("movq %0, %%rdx" : : "r"(setFrom->rdx): "rdx");
    __asm__ __volatile__("movq %0, %%rsi" : : "r"(setFrom->rsi): "rsi");
    __asm__ __volatile__("movq %0, %%rdi" : : "r"(setFrom->rdi): "rdi");
    __asm__ __volatile__("movq %0, %%rsp" : : "g"(setFrom->rsp): "rsp");
    __asm__ __volatile__("movq %0, %%rbp" : : "m"(setFrom->rbp): "rax", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11","memory", "cc");
    __asm__ __volatile__("movq %0, %%rax" : : "r"(setFrom->rip): "rax"); // put instruction pointer into rax
    __asm__ __volatile__("movq %rax, (%rsp)"); // move instruction pointer to rsp
    __asm__ __volatile__("movq %0, %%rax" : : "r"(setFrom->rax): "rax");
    __asm__ __volatile__("ret"); // return to instruction pointer
}

// Set all registers of previously yielded uthread
void setAllRegistersUThread(processRegisters* setFrom){
    __asm__ __volatile__("movq %0, %%r8" : : "r"(setFrom->r8): "r8");
    __asm__ __volatile__("movq %0, %%r9" : : "r"(setFrom->r9): "r9");
    __asm__ __volatile__("movq %0, %%r10" : : "r"(setFrom->r10): "r10");
    __asm__ __volatile__("movq %0, %%r11" : : "r"(setFrom->r11): "r11");
    __asm__ __volatile__("movq %0, %%r12" : : "r"(setFrom->r12): "r12");
    __asm__ __volatile__("movq %0, %%r13" : : "r"(setFrom->r13): "r13");
    __asm__ __volatile__("movq %0, %%r14" : : "r"(setFrom->r14): "r14");
    __asm__ __volatile__("movq %0, %%r15" : : "r"(setFrom->r15): "r15");
    __asm__ __volatile__("movq %0, %%rax" : : "r"(setFrom->rax): "rax");
    __asm__ __volatile__("movq %0, %%rbx" : : "r"(setFrom->rbx): "rbx");
    __asm__ __volatile__("movq %0, %%rcx" : : "r"(setFrom->rcx): "rcx");
    __asm__ __volatile__("movq %0, %%rdx" : : "r"(setFrom->rdx): "rdx");
    __asm__ __volatile__("movq %0, %%rsi" : : "r"(setFrom->rsi): "rsi");
    __asm__ __volatile__("movq %0, %%rdi" : : "r"(setFrom->rdi): "rdi");
    __asm__ __volatile__("movq %0, %%rsp" : : "r"(setFrom->rsp): "rsp");
    __asm__ __volatile__("movq %0, %%rbp" : : "r"(setFrom->rbp): "rax", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11","memory", "cc");
    __asm__ __volatile__("movq %0, %%rax" : : "r"(setFrom->rip): "rax");
}

// Gives every thread it's own rsp and rbp before you start it. This is because, if this thread is yielded, it could be run on multiple kernel threads, so it needs its OWN stack to not access illegal memory!
void setInitialUthreadRegs(processRegisters* setFrom){
    void* uthreadsPersonalStack;
    int ret1 = posix_memalign(&uthreadsPersonalStack, getpagesize(), 17*1024);
    void **whackstack1 = (void **)(uthreadsPersonalStack + 17*1024);
    whackstack1[-1] = nullptr;
    setFrom->rbp = static_cast<char *>(static_cast<void *>(whackstack1)) - (sizeof(char *) * 2);
    setFrom->rsp = static_cast<char *>(static_cast<void *>(whackstack1)) - (sizeof(char *) * 2);
}

// Call the function that the user wants using pure inlined assembly, so that we can set the rsp and rbp without causing error
void initalUthreadCall(processRegisters* setFrom){
    __asm__ __volatile__(
        "movq 72(%rdi), %rbp\n"    // Set initial frame pointer
        "movq 64(%rdi), %rsp\n"    // Set initial stack pointer
        "push 128(%rdi)\n" // Push value of function argument
        "movq 80(%rdi), %rax\n" // Move instruction pointer to rax, so you can jump from it
        "movq 128(%rdi), %rdi\n" //Move value of function argument
        "jmp     *%rax" // Jump to value stored in rax
    );
        
    goto *setFrom->rip; 
}

// Set an instruction pointer on top of stack and return to it. Isn't used anymore, but does work
void jumpToSavedRIP(processRegisters* setFrom){
    __asm__ __volatile__("movq %0, %%rax" : : "r"(setFrom->rip): "rax");
    __asm__ __volatile__("movq %rax, (%rsp)");
    __asm__ __volatile__("ret");
}

// This kernel thread is finished with its tasks, and the user wants all threads to exit when finished
void kernel_thread_exit(void){
    if(verbosey){
        printf("Exiting kernel thread\n");
    }
    kernelThreadsFinished++;
    while(kernelThreadsFinished < 5){
        pthread_yield();
    }
}

// The main handler function that our four kernel threads will run. Functions as a scheduler.
void* handler(void* arg) {
    kernelThreadObject* dad = new kernelThreadObject;
    processRegisters* pReggy = new processRegisters;
    pReggy->rip = (void *) handler;
    dad->pRegs = pReggy;
    threadObject* currentThreadObj;
    while(true) {
        // Save registers of parent thread so we can jump back later if this thread is yielded
        saveAllRegisters(dad->pRegs);

        if(usePriorityQueue){
            currentThreadObj = priorityThreadWaitingQueue.pop();
        }
        else{
            currentThreadObj = threadWaitingQueue.pop();
        }

        if(currentThreadObj->finishSignal){
            kernel_thread_exit();
        }
        currentThreadObj->kernelThreadBelongsTo = dad;
        current_uthread = currentThreadObj;
        if(verbosey){
            printf("Pulled off thread %lu \n", (unsigned long)current_uthread->arg);
        }
        else{
            printf("");
        }

        // Now run the process
        if(current_uthread->hasBeenRun == true){
            // This user thread has been run before, lets resume it by setting its registers
            setAllRegistersUThread(current_uthread->pRegs);
        }
        else{
            // This user thread hasn't been run before, let's intialize it in a safe way
            saveAllRegisters(current_uthread->pRegs);
            current_uthread->hasBeenRun = true;
            setInitialUthreadRegs(current_uthread->pRegs);
            current_uthread->pRegs->rip = (void *)current_uthread->func;
            current_uthread->pRegs->rdi = (void *)current_uthread->arg;
            current_uthread->pRegs->handlerAddr = (void *)handler;
            initalUthreadCall(current_uthread->pRegs);
        }

        // Now that we are finished with task, if we are now requested to stop by the main thread, exit
        if(usePriorityQueue){
            if(priorityThreadWaitingQueue.isEmpty() && whenFinshedJustStop){
                kernel_thread_exit();
            }
        }
        else{
            if(threadWaitingQueue.isEmpty() && whenFinshedJustStop){
                kernel_thread_exit();
            }
        }
    }
    return NULL;
}

// Initializing our four kernel threads
void uthread_init(void){
    pthread_create(& kernelThread1.threadId, NULL, (void* (*)(void*))handler, NULL);
    pthread_create(& kernelThread2.threadId, NULL, (void* (*)(void*))handler, NULL);
    pthread_create(& kernelThread3.threadId, NULL, (void* (*)(void*))handler, NULL);
    pthread_create(& kernelThread4.threadId, NULL, (void* (*)(void*))handler, NULL);
}

// Create a virtual thread for the user
void uthread_create(void (*func) (void*), void* arg){
    // Create a TCB for this thread, and add it to our scheduler
    totalUserThreadsCreated++;
    pthread_t pThreadId;
    processRegisters* kernProcRegs = new processRegisters;
    threadObject* tmp = new threadObject{func, arg, pThreadId, nullptr, 0, kernProcRegs, nullptr, 0, false, false};
    if(usePriorityQueue){   
        priorityThreadWaitingQueue.push(tmp);
    }
    else{
        threadWaitingQueue.push(tmp);
    }
}

// The user isn't going to use their virtual thread anymore, so jump kernel thread back to handler
void uthread_exit(void){
    handler(NULL);
}

// The user wants to yield the thread, so that one of our kernel threads can schedule a differnt virtual user thread to run.
void uthread_yield(void){
    if(verbosey){
        printf("Thread %lu yielded\n", (unsigned long)current_uthread->arg);
    }
    else{
        printf("");
    }
    
    // Save registers of currently running thread
    saveAllRegisters(current_uthread->pRegs);
    
    // Add TCB back to scheduler
    if(usePriorityQueue){   
        priorityThreadWaitingQueue.push(current_uthread);
    }
    else{
        threadWaitingQueue.push(current_uthread);
    }

    // Set registers back to our handler
    setAllRegisters(current_uthread->kernelThreadBelongsTo->pRegs);
}

// The user wants to exit after all of their tasks are completed
void uthread_cleanup(void){
    if(verbosey){
        printf("Signal to stop after tasks finished called\n");
    }

    whenFinshedJustStop = true;
    if(usePriorityQueue){
        priorityThreadWaitingQueue.setFullStop(true);
    }
    else{
        threadWaitingQueue.setFullStop(true);
    }

    if(totalUserThreadsCreated < 4){
        // If less than four user threads were created, our kernel threads will be stuck popping the queue, so add terminal entries
        threadObject* tmp = new threadObject;
        tmp->finishSignal = true;
        for(int it = totalUserThreadsCreated; it < 4; it++){
            if(usePriorityQueue){   
                priorityThreadWaitingQueue.push(tmp);
            }
            else{
                threadWaitingQueue.push(tmp);
            }
        }
    }
    
    while(kernelThreadsFinished < 4){
        pthread_yield();
    }

    if(verbosey){
        printf("All kernel threads done, exiting\n");
    }
    whenFinshedJustStop = false;
    return;
}

// Let the user set Priority based scheduler or First Come First Serve
void uthread_set_policy(enum uthread_policy policy){
    pthread_attr_t threadAttributes;
    pthread_attr_init(& threadAttributes);

    if(pthread_attr_setschedpolicy(& threadAttributes, policy) != 0){
        std::cout << "Wasn't able to set thread policy!" << std::endl;
    }
    
    // If policy is priority, change FCFS serve queue to a priority queue
    if(policy == UTHREAD_PRIORITY){
        if(verbosey){ 
            printf("Using Priority Queue\n");
        }
        else{
            printf("");
        }
        usePriorityQueue = true;
    }
}

// Set priority of a thread
void uthread_set_param(int param){
    current_uthread->priority = param;
}

// Log verbose messages
void setVerbose(bool inp){
    verbosey = inp;
}
#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <pthread.h>

struct processRegisters{
    void *r8; //0
    void *r9; //8
    void *r10; //16
    void *r11; //24
    void *r12; //32
    void *r13; //40
    void *r14; //48
    void *r15; //56
    void *rsp; //64
    void *rbp; //72
    void *rip; //80,  instruction pointers
    void *rbx; //88
    void *rax; //96
    void *rcx; //104
    void *rdx; //112
    void *rsi; //120
    void *rdi; //128
    void *pp;
    void *handlerAddr;
};

struct kernelThreadObject{
    void (*func) (void*);
    void* arg;
    pthread_t threadId;
    void* stack;
    processRegisters* pRegs;
    bool everAssigned = false;
};

struct threadObject{
    void (*func) (void*);
    void* arg;
    pthread_t threadId;
    void* stack;
    int running; // if running = 0, then it hasn't been scheduled yet. If running = 1, then it has been scheduled before. 
    processRegisters* pRegs;
    kernelThreadObject* kernelThreadBelongsTo;
    int priority;
    bool hasBeenRun;
    bool finishSignal;
    bool operator<(const threadObject &o) const {
        return priority < o.priority;
    }
};

struct threadObjectPointer_comparison {
    bool operator () ( const threadObject* a, const threadObject* b ) const {
        return a->priority > b->priority;
    }
};
void uthread_init(void);

void uthread_create(void (*func) (void*), void* arg);

void uthread_exit(void);

void uthread_yield(void);

void uthread_cleanup(void);

enum uthread_policy {
    UTHREAD_DIRECT_PTHREAD = 0,
    UTHREAD_PRIORITY,
};

void uthread_set_policy(enum uthread_policy policy);

void uthread_set_param(int param);

void setVerbose(bool inp);
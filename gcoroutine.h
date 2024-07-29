#ifndef __GCO_ROUTINE_H__
#define __GCO_ROUTINE_H__

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

typedef void(*fn_coroutine)(struct CoScheduler*, void *);
struct Stack;
class StackPool;

enum coroutine_status {
        READY = 0,
        RUNNING,
        SUSPEND,
        ENDED,
        EXITED
};


struct Coroutine
{
    CoScheduler* co_sch;
    fn_coroutine func;
    void* args;
    ucontext_t context;
    coroutine_status status;
    Stack* st_mem;
};

struct CoScheduler
{
    StackPool* stack_pool;
    ucontext_t main_ctx;
    Coroutine** coroutines;
    int nco;
    int cap;
    int running;
};

struct CoScheduler* coscheculer_open();

void coscheculer_close(CoScheduler* S);

int coroutine_new(CoScheduler* S, fn_coroutine func, void* args);

void coroutine_resume(CoScheduler* S, int id);

void coroutine_yield(CoScheduler* S);

void coroutine_close(CoScheduler* S, Coroutine*);

coroutine_status coroutine_check(CoScheduler* S, int id);
#endif
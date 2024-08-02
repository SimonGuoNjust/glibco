#ifndef __GCO_ROUTINE_H__
#define __GCO_ROUTINE_H__
#ifdef USE_BOOST_CONTEXT
#include <boost/context/all.hpp>
using namespace boost::context::detail;
#else
#include <ucontext.h>
#endif

#include <stdio.h>
#include <stdlib.h>


typedef void(*fn_coroutine)(void *);

// using fn_coroutine = std::function<void(struct Coroutine*, void*)>;
struct Stack;
struct CoScheduler;
using CoroutineContext = void*;
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
    // CoScheduler* co_sch;
    coroutine_status status;
    fn_coroutine func;
    void* args;
    CoroutineContext co_ctx;
    CoroutineContext main_ctx;
    // Stack* st_mem;
};

struct jump_data
{
    Coroutine* co;
    fcontext_t main_ctx;
};

struct func_args
{
	Coroutine* co;
	void* args;
};

struct CoScheduler
{
    StackPool* stack_pool;
    CoroutineContext* return_ctx;
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
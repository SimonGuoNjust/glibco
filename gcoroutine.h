#ifndef __GCO_ROUTINE_H__
#define __GCO_ROUTINE_H__
#include <boost/context/detail/fcontext.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
using namespace boost::context::detail;

typedef void(*fn_coroutine)(transfer_t);

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

class Coroutine
{
private:
    fn_coroutine fn_;
    void* args_;
    Stack* sctx_;
    StackPool* salloc_;
    void destroy(Coroutine* c);

public:
    fcontext_t co_ctx_;
    coroutine_status status;
    Coroutine(Stack* sctx, StackPool* alloc, fn_coroutine func, void* args):
        fn_(func),
        args_(args),
        sctx_(sctx),
        salloc_(alloc){
    }

    Coroutine( Coroutine const& ) = delete;
    Coroutine& operator=(Coroutine const&)= delete;
    fcontext_t run(fcontext_t fctx);
    void close();
    
};

class CoScheduler
{
private:
    CoScheduler();
    ~CoScheduler()
    {
        this->close();
    }

public:
    static CoScheduler& open()
    {
        static CoScheduler S;
        return S;
    }

    void close();
    CoScheduler(const CoScheduler&) = delete;
    CoScheduler& operator=(const CoScheduler&) = delete;
    StackPool* stack_pool;
    CoroutineContext* return_ctx;
    std::vector<Coroutine*> coroutines;
    int nco;
    int cap;
    int running;
};

struct CoScheduler* coscheculer_open();

void coscheculer_close(CoScheduler* S);

int coroutine_new(CoScheduler* S, fn_coroutine&& func, void* args);

void coroutine_resume(CoScheduler* S, int id);

fcontext_t coroutine_yield(transfer_t);

void coroutine_close(CoScheduler* S, Coroutine*);

coroutine_status coroutine_check(CoScheduler* S, int id);
#endif
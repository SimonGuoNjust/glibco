#ifndef __GCO_ROUTINE_H__
#define __GCO_ROUTINE_H__
#include <boost/context/detail/fcontext.hpp>
#include <../include/stackpool.h>
#include <stdio.h>
#include <stdlib.h>
using namespace boost::context::detail;

typedef void(*fn_coroutine)(class Coroutine*);

// using fn_coroutine = std::function<void(struct Coroutine*, void*)>;
struct Stack;
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
    fcontext_t main_ctx_;
    coroutine_status status;
    Coroutine() = default;
    Coroutine(Stack* sctx, StackPool* alloc, fn_coroutine func, void* args):
        fn_(func),
        args_(args),
        sctx_(sctx),
        salloc_(alloc){
    }

    Coroutine( Coroutine const& ) = delete;
    Coroutine& operator=(Coroutine const&)= delete;
    fcontext_t run();
    void close();
    void resume();
    fcontext_t yield();
    
};
template<typename Fn, typename StackPool>
Coroutine* coroutine_new(StackPool* S, Fn&& func, void* args);
// struct CoScheduler* coscheculer_open();

// void coscheculer_close(CoScheduler* S);

// int coroutine_new(CoScheduler* S, fn_coroutine&& func, void* args);

// void coroutine_resume(CoScheduler* S, int id);

fcontext_t coroutine_yield(transfer_t);

// void coroutine_close(CoScheduler* S, Coroutine*);

// coroutine_status coroutine_check(CoScheduler* S, int id);


void Coroutine::destroy(Coroutine* c)
{
	StackPool* alloc = c->salloc_;
	Stack* sctx = c->sctx_;
	c->~Coroutine();
	alloc->release_stack(sctx);
}

fcontext_t Coroutine::run()
{
	// transfer_t res_t{fctx};
	status = RUNNING;
	fn_(this);
	return jump_fcontext(main_ctx_, nullptr).fctx;
}

void Coroutine::close()
{
	destroy(this);
}

void func_wrapper(transfer_t t)
{
	Coroutine* c = static_cast<Coroutine*>(t.data);
	assert(nullptr != t.fctx);
	assert(nullptr != c);
	t = jump_fcontext(t.fctx, nullptr);
	c->main_ctx_ = t.fctx;
	t.fctx = c->run();
	c->status = ENDED;
	jump_fcontext(t.fctx, nullptr);
	// ontop_fcontext(t.fctx, c, on_coroutine_close);
}

template<typename Fn, typename StackPool>
Coroutine* coroutine_new(StackPool* S, Fn&& func, void* args)
{
	Stack* sctx = S->get_stack(__MINIMUM_BLOCKSIZE);
	void* sp = sctx->st_bottom;
	// size_t co_size = sizeof(Coroutine);
	void* co_space = reinterpret_cast< void * >(
			( reinterpret_cast< uintptr_t >( sp) - static_cast< uintptr_t >( sizeof( Coroutine) ) )
            & ~static_cast< uintptr_t >( 0xff) );
	// void* co_space = malloc(sizeof(Coroutine));
	Coroutine* new_co = new (co_space)Coroutine{sctx, S, func, args, };
	void * stack_top = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( co_space) - static_cast< uintptr_t >( 128) );
	const std::size_t size = reinterpret_cast< uintptr_t >(sp) - reinterpret_cast< uintptr_t >(sctx->st_top);
	const fcontext_t ctx = make_fcontext(stack_top, size, &func_wrapper);
	new_co->co_ctx_ = ctx;
	new_co->status = READY;
	// new_co->args_ = args;
	return new_co;
}

void Coroutine::resume()
{
	status = RUNNING;
	co_ctx_ = jump_fcontext(co_ctx_, this).fctx;
}

fcontext_t Coroutine::yield()
{
	status = SUSPEND;
	main_ctx_ = jump_fcontext(main_ctx_, nullptr).fctx;
}

#endif
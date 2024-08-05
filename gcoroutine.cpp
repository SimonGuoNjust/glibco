#include <gcoroutine_i.h>
#include <functional>
StackPool::StackPool(char is_shared)
{
    if (is_shared == 0)
    {
        memory_manager = new MemoryPool();
    }
}

Stack* StackPool::get_stack(unsigned int bytes)
{
    Stack* new_stack = (Stack*)malloc(sizeof(*new_stack));
    new_stack->bind_co = NULL;
    new_stack->size = bytes;
    // new_stack->st_top = (char*)memory_manager->get_buffer(bytes);
	new_stack->st_top = malloc(bytes);
    new_stack->st_bottom = static_cast< char * >( new_stack->st_top) + bytes;
    return new_stack;
}

void StackPool::release_stack(Stack* stack)
{
    // memory_manager->release_buffer(stack->st_top, stack->size);
	free(stack->st_top);
	stack->st_top=nullptr;
	stack->st_bottom=nullptr;
	stack->size=0;
}

StackPool::~StackPool()
{
    delete memory_manager;
}

void Coroutine::destroy(Coroutine* c)
{
	StackPool* alloc = c->salloc_;
	Stack* sctx = c->sctx_;
	c->~Coroutine();
	alloc->release_stack(sctx);
}

fcontext_t Coroutine::run(fcontext_t fctx)
{
	transfer_t res_t{fctx};
	status = RUNNING;
	fn_(res_t);
	// std::invoke(fn_, std::move(res_t), std::move(args_));
	return jump_fcontext(fctx, nullptr).fctx;
}

void Coroutine::close()
{
	destroy(this);
}

// transfer_t on_coroutine_close(transfer_t t)
// {
// 	Coroutine* c = static_cast<Coroutine*>(t.data);
// 	c->close();
// 	return {nullptr, nullptr};
// }
static void dummpy()
{
	int a;
}
void func_wrapper(transfer_t t)
{
	Coroutine* c = static_cast<Coroutine*>(t.data);
	assert(nullptr != t.fctx);
	assert(nullptr != c);
	t = jump_fcontext(t.fctx, nullptr);
	std::cout << "here" << std::endl;
	t.fctx = c->run(t.fctx);
	c->status = ENDED;
	jump_fcontext(t.fctx, nullptr);
	// ontop_fcontext(t.fctx, c, on_coroutine_close);
}

CoScheduler::CoScheduler()
{
	this -> nco = 0;
	this -> cap = __MAX_COROUTINES;
	this -> running = -1;
	this -> stack_pool = new StackPool();
	this->coroutines.resize(this->cap);
}

void CoScheduler::close()
{
	int i;
	for (i=0;i<this->cap;i++) {
		Coroutine* co = this->coroutines[i];
		if (co) {
			co->close();
			}
	};
	delete this->stack_pool;
	this->stack_pool = NULL;
}

void coroutine_close(CoScheduler* S, Coroutine* co)
{
    // S->stack_pool->release_stack(co->st_mem);
    // free(co->st_mem);
    free(co);
}

int coroutine_new(CoScheduler* S, fn_coroutine&& func, void* args)
{
	Stack* sctx = S->stack_pool->get_stack(__MINIMUM_BLOCKSIZE);
	void* sp = sctx->st_bottom;
	// size_t co_size = sizeof(Coroutine);
	void* co_space = reinterpret_cast< void * >(
			( reinterpret_cast< uintptr_t >( sp) - static_cast< uintptr_t >( sizeof( Coroutine) ) )
            & ~static_cast< uintptr_t >( 0xff) );
	// void* co_space = malloc(sizeof(Coroutine));
	Coroutine* new_co = new (co_space)Coroutine{sctx, S->stack_pool, func, args, };
	void * stack_top = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( co_space) - static_cast< uintptr_t >( 128) );
	const std::size_t size = reinterpret_cast< uintptr_t >(sp) - reinterpret_cast< uintptr_t >(sctx->st_top);
	const fcontext_t ctx = make_fcontext(stack_top, size, &func_wrapper);
	new_co->co_ctx_ = ctx;
	new_co->status = READY;
	// new_co->args_ = args;
	int i;
	for (i=0;i<S->coroutines.size();i++) {
		int id = (i+S->nco) % S->coroutines.size();
		if (S->coroutines[id] == NULL) {
			S->coroutines[id] = new_co;
			++S->nco;
			return id;
		}
	}
	S->coroutines.push_back(new_co);
	return S->coroutines.size() - 1;
}

void coroutine_resume(CoScheduler* S, int id)
{
    // assert(S->running == -1);
	// assert(id >=0 && id < S->cap);

    // 取出协程
	Coroutine *C = S->coroutines[id];
	if (C == NULL)
		return;
	transfer_t res_t;
	coroutine_status status = C->status;
	switch(status) {
	case SUSPEND:
	case RUNNING:
	case READY:
		S->running = id;
		C->status = RUNNING;
		res_t = jump_fcontext(C->co_ctx_, C);
		C->co_ctx_ = res_t.fctx;
		break;
	default:
		assert(0);
	}
	if (C->status != ENDED) C->status = SUSPEND;
	S->running = -1;
}

fcontext_t coroutine_yield(transfer_t t)
{
    // Coroutine* c = static_cast<Coroutine*>(t.data);
	// assert(nullptr != t.fctx);
	// assert(nullptr != c);
	// c->status = SUSPEND;
	return jump_fcontext(t.fctx, nullptr).fctx;
}

coroutine_status coroutine_check(CoScheduler* S, int id)
{
    assert(id>=0 && id < S->cap);
	if (S->coroutines[id] == NULL) {
		return ENDED;
	}
	return S->coroutines[id]->status;
}
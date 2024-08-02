#include <gcoroutine_i.h>

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
	new_stack->st_top = (char*)malloc(bytes);
    new_stack->st_bottom = new_stack->st_top + bytes;
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

#ifdef USE_BOOST_CONTEXT


static void
func_wrapper(transfer_t t) {
	struct Coroutine *C = reinterpret_cast<Coroutine*>(t.data);
	func_args args = {C, C->args}; 
	C->main_ctx = t.fctx;
	C->func((void*)&args);
	C->status = ENDED;
	jump_fcontext(t.fctx, nullptr);
}
#else
static void
func_wrapper(uint32_t low32, uint32_t hi32) {
	uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
	struct CoScheduler *S = (struct CoScheduler *)ptr;

	int id = S->running;
	struct Coroutine *C = S->coroutines[id];
	C->func(S, C->args);
	C->status = ENDED;
	coroutine_close(S, C);
	S->coroutines[id] = NULL;
	--S->nco;
	S->running = -1;
}
#endif
struct CoScheduler* coscheculer_open()
{
    CoScheduler* new_sch = (CoScheduler*)malloc(sizeof(*new_sch));
    new_sch -> nco = 0;
    new_sch -> cap = __MAX_COROUTINES;
    new_sch -> running = -1;
    new_sch -> coroutines = (Coroutine**)calloc(new_sch->cap, sizeof(Coroutine*));
	// #ifdef USE_BOOST_CONTEXT
	// new_sch -> return_ctx = (CoroutineContext*)calloc(new_sch->cap, sizeof(CoroutineContext));
	// #else
	// new_sch -> return_ctx = (CoroutineContext*)malloc(sizeof(CoroutineContext));
	// #endif
    new_sch -> stack_pool = new StackPool();
    return new_sch;
}

void coscheculer_close(CoScheduler* S)
{
    int i;
    for (i=0;i<S->cap;i++) {
		Coroutine* co = S->coroutines[i];

		if (co) {
			coroutine_close(S, co);
		}
	}
	free(S->coroutines);
	free(S->return_ctx);
    delete S->stack_pool;
	S->return_ctx = NULL;
	S->coroutines = NULL;
    S->stack_pool = NULL;
	free(S);
};

void coroutine_close(CoScheduler* S, Coroutine* co)
{
    // S->stack_pool->release_stack(co->st_mem);
    // free(co->st_mem);
    free(co);
}

int coroutine_new(CoScheduler* S, fn_coroutine func, void* args)
{
    Coroutine* new_co = (Coroutine*)malloc(sizeof(*new_co));
	memset(new_co, 0, sizeof(*new_co));
    new_co -> func = func;
    new_co -> args = args;
    // new_co -> co_sch = S;
    new_co -> status = READY;
    // new_co -> st_mem = S->stack_pool->get_stack(__MINIMUM_BLOCKSIZE);
    if (S->nco >= S->cap) {
		int id = S->cap;
		S->coroutines = (Coroutine**)realloc(S->coroutines, S->cap * 2 * sizeof(struct Coroutines *));
		memset(S->coroutines + S->cap , 0 , sizeof(struct Coroutine *) * S->cap);
		S->coroutines[S->cap] = new_co;
		S->cap *= 2;
		++S->nco; 
		return id;
	} else {
		int i;
		for (i=0;i<S->cap;i++) {
			int id = (i+S->nco) % S->cap;
			if (S->coroutines[id] == NULL) {
				S->coroutines[id] = new_co;
				++S->nco;
				return id;
			}
		}
	}
	return -1;
}

void coroutine_resume(CoScheduler* S, int id)
{
    assert(S->running == -1);
	assert(id >=0 && id < S->cap);

    // 取出协程
	struct Coroutine *C = S->coroutines[id];
	if (C == NULL)
		return;

	coroutine_status status = C->status;
	#ifdef USE_BOOST_CONTEXT
	transfer_t res_t;
	void* temp_st;
	#endif
    uintptr_t ptr;
	
	switch(status) {
	case READY:
		#ifdef USE_BOOST_CONTEXT
		temp_st = malloc(1024 * 16);
		// C->co_ctx = make_fcontext(C->st_mem->st_top, C->st_mem->size, func_wrapper);
		C->co_ctx = make_fcontext(temp_st, 1024 * 16,  func_wrapper);
		S->running = id;
		C->status = RUNNING;
		res_t = jump_fcontext(C->co_ctx, C);
		C->co_ctx = res_t.fctx;
		#else
		getcontext(&C->context);
		C->st_mem = S->stack_pool->get_stack(__MINIMUM_BLOCKSIZE * 2);
		C->context.uc_stack.ss_sp = C->st_mem->st_top;
		C->context.uc_stack.ss_size = __MINIMUM_BLOCKSIZE * 2;
		C->context.uc_link = S->return_ctx;

		S->running = id;
		C->status = RUNNING;
		// 设置执行C->ctx函数, 并将S作为参数传进去
		ptr = (uintptr_t)S;
		makecontext(&C->context, (void (*)(void)) func_wrapper, 2, (uint32_t)ptr, (uint32_t)(ptr>>32));
		// 将当前的上下文放入S->main中，并将C->ctx的上下文替换到当前上下文
		swapcontext(S->return_ctx, &C->context);
		#endif
		break;
	case SUSPEND:
		S->running = id;
		C->status = RUNNING;
		#ifdef USE_BOOST_CONTEXT
		res_t = jump_fcontext(C->co_ctx, C);
		C->co_ctx = res_t.fctx;
		#else
		swapcontext(S->return_ctx, &C->context);
		#endif
		break;
	default:
		assert(0);
	}
	if (C->status == RUNNING) C->status = SUSPEND;
	S->running = -1;
}

void coroutine_yield(CoScheduler* S)
{
    int id = S->running;
	assert(id >= 0);
	// struct Coroutine * C = activate_co;
	// if (C == nullptr)
	// {
	// 	return;
	// }
	struct Coroutine * C = S->coroutines[id];
	// jump_data jmp_data = {S, nullptr};
	C->status = SUSPEND;
	S->running = -1;
	#ifdef USE_BOOST_CONTEXT
	// fcontext_t return_des = S->return_ctx[id];
	transfer_t res_t = jump_fcontext(C->main_ctx, nullptr);
	// S->return_ctx[id] = res_t.fctx;
	#else
	swapcontext(&C->context , &S->main_ctx);
	#endif
}

coroutine_status coroutine_check(CoScheduler* S, int id)
{
    assert(id>=0 && id < S->cap);
	if (S->coroutines[id] == NULL) {
		return ENDED;
	}
	return S->coroutines[id]->status;
}
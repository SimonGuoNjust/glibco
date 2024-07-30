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
    new_stack->st_top = (char*)memory_manager->get_buffer(bytes);
    new_stack->st_bottom = new_stack->st_top + bytes;
    return new_stack;
}

void StackPool::release_stack(Stack* stack)
{
    memory_manager->release_buffer(stack->st_top, stack->size);
}

StackPool::~StackPool()
{
    delete memory_manager;
}

static void
func_wrapper(uint32_t low32, uint32_t hi32) {
	uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
	struct CoScheduler *S = (struct CoScheduler *)ptr;

	int id = S->running;
	struct Coroutine *C = S->coroutines[id];
	C->func(S, C->args);	// 中间有可能会有不断的yield
	C->status = ENDED;
	coroutine_close(S, C);
	S->coroutines[id] = NULL;
	--S->nco;
	S->running = -1;
}

struct CoScheduler* coscheculer_open()
{
    CoScheduler* new_sch = (CoScheduler*)malloc(sizeof(*new_sch));
    new_sch -> nco = 0;
    new_sch -> cap = __MAX_COROUTINES;
    new_sch -> running = -1;
    new_sch -> coroutines = (Coroutine**)calloc(new_sch->cap, sizeof(Coroutine*));
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
    delete S->stack_pool;
	S->coroutines = NULL;
    S->stack_pool = NULL;
	free(S);
};

void coroutine_close(CoScheduler* S, Coroutine* co)
{
    S->stack_pool->release_stack(co->st_mem);
    free(co->st_mem);
    free(co);
}

int coroutine_new(CoScheduler* S, fn_coroutine func, void* args)
{
    Coroutine* new_co = (Coroutine*)malloc(sizeof(*new_co));
    new_co -> func = func;
    new_co -> args = args;
    new_co -> co_sch = S;
    new_co -> status = READY;
    new_co -> st_mem = NULL;
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
    uintptr_t ptr;
	switch(status) {
	case READY:
	    //初始化ucontext_t结构体,将当前的上下文放到C->ctx里面
		getcontext(&C->context);
		C->st_mem = S->stack_pool->get_stack(__MINIMUM_BLOCKSIZE * 2);
		C->context.uc_stack.ss_sp = C->st_mem->st_top;
		C->context.uc_stack.ss_size = __MINIMUM_BLOCKSIZE * 2;
		C->context.uc_link = &S->main_ctx;

		S->running = id;
		C->status = RUNNING;
		// 设置执行C->ctx函数, 并将S作为参数传进去
		ptr = (uintptr_t)S;
		makecontext(&C->context, (void (*)(void)) func_wrapper, 2, (uint32_t)ptr, (uint32_t)(ptr>>32));
		// 将当前的上下文放入S->main中，并将C->ctx的上下文替换到当前上下文
		swapcontext(&S->main_ctx, &C->context);
		break;
	case SUSPEND:
		S->running = id;
		C->status = RUNNING;
		swapcontext(&S->main_ctx, &C->context);
		break;
	default:
		assert(0);
	}
}

void coroutine_yield(CoScheduler* S)
{
    int id = S->running;
	assert(id >= 0);

	struct Coroutine * C = S->coroutines[id];

	C->status = SUSPEND;
	S->running = -1;

	swapcontext(&C->context , &S->main_ctx);
}

coroutine_status coroutine_check(CoScheduler* S, int id)
{
    assert(id>=0 && id < S->cap);
	if (S->coroutines[id] == NULL) {
		return ENDED;
	}
	return S->coroutines[id]->status;
}
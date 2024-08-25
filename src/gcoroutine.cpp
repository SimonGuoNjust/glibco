#include <../include/gcoroutine.h>

// void coroutine_resume(CoScheduler* S, int id)
// {
//     // assert(S->running == -1);
// 	// assert(id >=0 && id < S->cap);

//     // 取出协程
// 	Coroutine *C = S->coroutines[id];
// 	if (C == NULL)
// 		return;
// 	transfer_t res_t;
// 	coroutine_status status = C->status;
// 	switch(status) {
// 	case SUSPEND:
// 	case RUNNING:
// 	case READY:
// 		S->running = id;
// 		C->status = RUNNING;
// 		res_t = jump_fcontext(C->co_ctx_, C);
// 		C->co_ctx_ = res_t.fctx;
// 		break;
// 	default:
// 		assert(0);
// 	}
// 	if (C->status != ENDED) C->status = SUSPEND;
// 	S->running = -1;
// }

// fcontext_t coroutine_yield(transfer_t t)
// {
//     // Coroutine* c = static_cast<Coroutine*>(t.data);
// 	// assert(nullptr != t.fctx);
// 	// assert(nullptr != c);
// 	// c->status = SUSPEND;
// 	return jump_fcontext(t.fctx, nullptr).fctx;
// }

// coroutine_status coroutine_check(CoScheduler* S, int id)
// {
//     assert(id>=0 && id < S->cap);
// 	if (S->coroutines[id] == NULL) {
// 		return ENDED;
// 	}
// 	return S->coroutines[id]->status;
// }
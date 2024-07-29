#include "gcoroutine.h"
#include <iostream>

struct args {
	int n;
};

static void
foo(struct CoScheduler * S, void *ud) {
	struct args * arg = (args*)ud;
	int start = arg->n;
	int i;
	for (i=0;i<5;i++) {
		std::cout << "coroutine "<< S->running <<  " " << start + i << std::endl;
		coroutine_yield(S);
	}
}

static void
test(struct CoScheduler *S) {
	struct args arg1 = { 0 };
	struct args arg2 = { 100 };

	// 创建两个协程
	int co1 = coroutine_new(S, foo, &arg1);
	int co2 = coroutine_new(S, foo, &arg2);

	printf("main start\n");
	while (coroutine_check(S, co1) != ENDED && coroutine_check(S, co2) != ENDED) {
		// 使用协程co1
		coroutine_resume(S,co1);
		// 使用协程co2
		coroutine_resume(S,co2);
	} 
	printf("main end\n");
}

int 
main() {
	// 创建一个协程调度器
	struct CoScheduler * S = coscheculer_open();
	test(S);

	// 关闭协程调度器
	coscheculer_close(S);
	
	return 0;
}


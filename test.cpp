#include "gcoroutine.h"
#include <iostream>
// #include <chrono>
// #define CALC_CLOCK_T std::chrono::system_clock::time_point
// #define CALC_CLOCK_NOW() std::chrono::system_clock::now()
// #define CALC_MS_CLOCK(x) static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(x).count())
// #define CALC_NS_AVG_CLOCK(x, y) \
//     static_cast<long long>(std::chrono::duration_cast<std::chrono::nanoseconds>(x).count() / (y ? y : 1))
// extern Coroutine* active_co;
struct args {
	int n;
};

static void
foo(void *ud) {
	struct func_args * arg = reinterpret_cast<func_args*>(ud);
	Coroutine* co = arg->co;
	// int start = arg->n;
	int i;
	for (i=0;i<5;i++) {
		// std::cout << "coroutine "<< S->running <<  " " << start + i << std::endl;
		// this_co->status = SUSPEND;
		transfer_t t = jump_fcontext(co->main_ctx, nullptr);
		// co->main_ctx = t.fctx;
		// coroutine_yield();
	}
}

// static void
// test(struct CoScheduler *S) {
// 	struct args arg1 = { 0 };
// 	struct args arg2 = { 100 };

// 	// 创建两个协程
// 	int co1 = coroutine_new(S, foo, &arg1);
// 	int co2 = coroutine_new(S, foo, &arg2);

// 	printf("main start\n");
// 	while (coroutine_check(S, co1) != ENDED && coroutine_check(S, co2) != ENDED) {
// 		// 使用协程co1
// 		coroutine_resume(S,co1);
// 		// 使用协程co2
// 		coroutine_resume(S,co2);
// 	} 
// 	printf("main end\n");
// }

int 
main() {
	// 创建一个协程调度器
	struct CoScheduler * S = coscheculer_open();

	// time_t begin_time = time(nullptr);
	// CALC_CLOCK_T begin_clock = CALC_CLOCK_NOW();
	int max_coroutine_number = 10;
	int id = 0;
	for (int i = 0; i < max_coroutine_number; ++i) {
		struct args p = {i};
    	id = coroutine_new(S, foo, &p);
		id *= 1;
		// std::cout << "coroutine " << id
  	}
	// time_t end_time = time(nullptr);
	// CALC_CLOCK_T end_clock = CALC_CLOCK_NOW();
	// printf("create %d coroutine, cost time: %d s, clock time: %d ms, avg: %lld ns\n", max_coroutine_number,
    //      static_cast<int>(end_time - begin_time), CALC_MS_CLOCK(end_clock - begin_clock),
    //      CALC_NS_AVG_CLOCK(end_clock - begin_clock, max_coroutine_number));
	// test(S);
//   begin_time = end_time;
//   begin_clock = end_clock;
  bool continue_flag = true;
  long long real_switch_times = static_cast<long long>(0);
  while (continue_flag) {
    continue_flag = false;
    for (int i = 0; i < max_coroutine_number; ++i) {
      if (S->coroutines[i] && ENDED != S->coroutines[i]->status) {
		// if (S->coroutines[i]->status == READY)
		// {
		// 	begin_time = time(nullptr);
		// 	begin_clock = CALC_CLOCK_NOW();
		// }
        continue_flag = true;
        ++real_switch_times;
        coroutine_resume(S, i);
      }
	  else if (ENDED == S->coroutines[i]->status)
	  {
		coroutine_close(S, S->coroutines[i]);
		S->coroutines[id] = NULL;
		--S->nco;
	  }
    }
  }
//   end_time = time(nullptr);
//   end_clock = CALC_CLOCK_NOW();
//   printf("switch %d coroutine contest %lld times, cost time: %d s, clock time: %d ms, avg: %lld ns\n",
//          max_coroutine_number, real_switch_times, static_cast<int>(end_time - begin_time),
//          CALC_MS_CLOCK(end_clock - begin_clock), CALC_NS_AVG_CLOCK(end_clock - begin_clock, real_switch_times));

// 关闭协程调度器
    coscheculer_close(S);
	
	return 0;
}


#include "gcoroutine.hpp"
#include "scheduler.hpp"
#include "multithread_scheduler.hpp"
#include <iostream>
#include <chrono>
// #define TIMEOUTPUT
using fn_type = void(*)(Coroutine*);
#define CALC_CLOCK_T std::chrono::system_clock::time_point
#define CALC_CLOCK_NOW() std::chrono::system_clock::now()
#define CALC_MS_CLOCK(x) static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(x).count())
#define CALC_NS_AVG_CLOCK(x, y) \
    static_cast<long long>(std::chrono::duration_cast<std::chrono::nanoseconds>(x).count() / (y ? y : 1))
// extern Coroutine* active_co;
struct args {
	int n;
};

// int foo_1()
// {
// 	int a = 0;
// 	int b = 0;
// 	printf("s");
// 	a = a+b;
// 	return a;
// }

void
foo(Coroutine* c) {
	// int start = *reinterpret_cast<int*>(ud);
	int start = 0;
	srand(time(0));
	int s = rand() % 10;
	int * a = &start;
	// int * b = &i;
	for (int i=0;i<s;i++) {
		// foo_1();
		// this_co->status = SUSPEND;
		// t.fctx = coroutine_yield(t);
		// printf("%d : %d\n", i, s);
		 vfprintf_s(stdout, "a", nullptr);
		register_timeout<gCoScheduler>(20);
		// c->yield();
		// co->main_ctx = t.fctx;
		// coroutine_yield();
	}

	std::cout << "foo ended" << std::endl;
}




// int 
// main(int argc, char* argv[]) {
// 	// 创建一个协程调度器
// 	CoScheduler<Coroutine,StackPool>* S = &CoScheduler<Coroutine, StackPool>::open(); 
// 	#ifdef TIMEOUTPUT
// 	time_t begin_time = time(nullptr);
// 	CALC_CLOCK_T begin_clock = CALC_CLOCK_NOW();
// 	#endif
// 	int num_co = 30;
// 	if (argc > 1)
// 	{
// 		num_co = std::stoi(argv[1]);
// 	}
// 	int max_coroutine_number = num_co;
// 	int id = 0;
// 	for (int i = 0; i < max_coroutine_number; ++i) {
// 		// int p = {i};
//     	// id = coroutine_new(S, foo, nullptr);
// 		// id *= 1;
// 		// Coroutine* new_co = coroutine_new<fn_type>(S->stack_pool, std::forward<fn_type>(foo), nullptr);
// 		S->new_coroutine(foo, nullptr);
// 		// std::cout << "coroutine " << id
//   	}
// 	#ifdef TIMEOUTPUT
// 	time_t end_time = time(nullptr);
// 	CALC_CLOCK_T end_clock = CALC_CLOCK_NOW();
// 	printf("create %d coroutine, cost time: %d s, clock time: %d ms, avg: %lld ns\n", max_coroutine_number,
//          static_cast<int>(end_time - begin_time), CALC_MS_CLOCK(end_clock - begin_clock),
//          CALC_NS_AVG_CLOCK(end_clock - begin_clock, max_coroutine_number));	
// 	begin_time = end_time;
// 	begin_clock = end_clock;
// 	#endif
// //   bool continue_flag = true;
//   long long real_switch_times = static_cast<long long>(0);
// //   while (continue_flag) {
// //     continue_flag = false;
// //     for (int i = 0; i < max_coroutine_number; ++i) {
// //       if (S->coroutines[i]  && ENDED != S->coroutines[i]->status) {
		
// // 		if (S->coroutines[i]->status == READY)
// // 		{
// // 			begin_time = time(nullptr);
// // 			begin_clock = CALC_CLOCK_NOW();
// // 		}
// //         continue_flag = true;
// //         ++real_switch_times;
// // 		S->coroutines[i]->resume();
// // //         // coroutine_resume(S, i);
// //       }
// // 	  if (S->coroutines[i]  && ENDED == S->coroutines[i]->status)
// // 	  {
// // 		S->coroutines[i]->close();
// // 		S->coroutines[i] = nullptr;
// // 	  }
// //     }
// //   }
// 	// 	begin_time = end_time;
// 	// begin_clock = end_clock;
// 	real_switch_times = S->loop();
//   #ifdef TIMEOUTPUT
//   end_time = time(nullptr);
//   end_clock = CALC_CLOCK_NOW();
//   printf("switch %d coroutine contest %lld times, cost time: %d s, clock time: %d ms, avg: %lld ns\n",
//          max_coroutine_number, real_switch_times, static_cast<int>(end_time - begin_time),
//          CALC_MS_CLOCK(end_clock - begin_clock), CALC_NS_AVG_CLOCK(end_clock - begin_clock, real_switch_times));
//   #endif
// // 关闭协程调度器
//     // coscheculer_close(S);
	
// 	return 0;
// }

int 
main(int argc, char* argv[]) {
	// 创建一个协程调度器
	// auto sp = std::make_shared<StackPool>();
    ThreadPoolCoScheduler<Coroutine, StackPool> S;
	// S.init(sp);
	S.init();
// 	#ifdef TIMEOUTPUT
// 	time_t begin_time = time(nullptr);
// 	CALC_CLOCK_T begin_clock = CALC_CLOCK_NOW();
// 	#endif
	int num_co = 30;
	if (argc > 1)
	{
		num_co = std::stoi(argv[1]);
	}
	int max_coroutine_number = num_co;
	int id = 0;
	for (int i = 0; i < max_coroutine_number; ++i) {
		S.new_coroutine(foo, nullptr);
  	}
	S.start();
	S.run();
	return 0;
}
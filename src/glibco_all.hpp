#pragma once

#include "gcoroutine.hpp"
#include "epoll_scheduler.hpp"

typedef ThreadPoolCoScheduler<CoScheduler<Coroutine, StackPool>> NormalScheduler;
typedef ThreadPoolCoScheduler<EpollCoScheduler<Coroutine, StackPool>> EpollScheduler;
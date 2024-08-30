#pragma once

#include "gcoroutine.hpp"
#include "multithread_scheduler.hpp"

typedef ThreadPoolCoScheduler<CoScheduler<Coroutine, StackPool>> NormalScheduler;

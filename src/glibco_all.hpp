#pragma once

#include "gcoroutine.hpp"
#include "multithread_scheduler.hpp"

typedef ThreadPoolCoScheduler<Coroutine, StackPool> GCoScheduler;
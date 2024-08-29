#ifndef __MULTITHREAD_SCHEDULER_H
#define __MULTITHREAD_SCHEDULER_H

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <thread>
#include <iostream>
#include <atomic>
#include <functional>
#include <condition_variable>
#include "scheduler.hpp"

typedef void(*fn_thread_task)(void*);

class spin_mutex {
	std::atomic<bool> flag = ATOMIC_VAR_INIT(false);
public:
	spin_mutex() = default;
	spin_mutex(const spin_mutex&) = delete;
	spin_mutex& operator= (const spin_mutex&) = delete;
	void lock() {
		bool expected = false;
		while (!flag.compare_exchange_strong(expected, true))
			expected = false;
	}
	void unlock() {
		flag.store(false);
	}
};

class ThreadPool
{
enum ThreadStatus{
    // sleep wait(set) or busy wait(unset)
    isSleep = 1, 
    // thread running(set) or wait(unset)
    isRunning = 2, 
     // has task(set) or not(unset)
    isSuspend = 4,
};

class ThreadInfo
{
public:
    std::condition_variable cond;
    std::mutex mutex_task;
    spin_mutex mutex_spin;
    std::vector<char> thread_status;
    fn_thread_task func;
    std::vector<void*> args;
    ThreadInfo(int num_threads)
    {
        func = nullptr;
        thread_status.resize(num_threads, isSuspend);
        args.resize(num_threads, nullptr);
    }
    
    inline bool checkStatus(int id, char status_) const
    {
        return (thread_status[id] & status_) == status_;
    }

    inline void unset(int id, char status_)
    {
        thread_status[id] &= ~(status_);
    }

    inline void set(int id, char status_)
    {
        thread_status[id] |= status_;
    }
    
};

public:
    ThreadPool(): num_threads(6), thInfos(6) {}
    ThreadPool(int num_th): num_threads(num_th), thInfos(num_th) {}
    void initPool()
    {
        for (int i = 0; i < num_threads; i++)
        {
            pool_.emplace_back(std::bind(&ThreadPool::run, this, i));
        }
    }

    void run(int id)
    {
        ThreadInfo* th_infos = &(this->thInfos);
        for(;;)
        {
            if (th_infos->checkStatus(id, isSleep))
            {
                printf("Thread %d is sleeping \n", id); 
                std::unique_lock<std::mutex> lock(th_infos ->mutex_task);
                while(!(th_infos->checkStatus(id, isRunning | isSuspend)))
                {
                    th_infos->cond.wait(lock);
                }
                lock.unlock();
            }
            else
            {
                printf("Thread %d is busy waiting \n", id); 
                while(1)
                {
                    std::unique_lock<spin_mutex> lock(th_infos->mutex_spin);
                    if (th_infos->checkStatus(id, isRunning | isSuspend) || th_infos->checkStatus(id, isSleep))
                    {
                        break;
                    }
                }
            }

            if (!running)
            {
                printf("Thread %d is quit \n", id); 
                std::this_thread::sleep_for(std::chrono::microseconds(2000));
                break;
            }

            if (th_infos->checkStatus(id, isRunning))
            {
                printf("Thread %d is working \n", id); 
                th_infos->func(th_infos->args[id]);
                // th_infos->func[id](nullptr);
                std::lock_guard<std::mutex> lock(th_infos->mutex_task);
                th_infos->unset(id, isSuspend);
                // printf("%d \n", thInfos.checkStatus(id, isSuspend));
            }
        }
    }
   
    void add_task(void* args)
    {
        for (int i = 0; i < num_threads; i++)
        {
            if (add_task(i, args))
            {
                break;
            }
        }
    }

    bool add_task(int id, void* args)
    {
        if (!thInfos.checkStatus(id, isRunning))
        {
            thInfos.args[id] = args;
            thInfos.set(id, isSleep);
            thInfos.set(id, isRunning);
            thInfos.cond.notify_all();
            return true;
        }
        return false; 
    }

    template<typename Fn>
    void configure_task(Fn&& func)
    {
        thInfos.func = func;
    }

    void wait()
    {
        while(1)
        {
            std::lock_guard<std::mutex> lock(thInfos.mutex_task);
            bool AllDone = true;
            for (int i = 0; i < num_threads; i++)
            {
                if (thInfos.checkStatus(i, isRunning))
                {
                    if (thInfos.checkStatus(i, isSuspend)) AllDone = false;
                }
            }
            if (AllDone)
            {
                for (int i = 0; i < num_threads; i++)
                {
                    thInfos.unset(i, isRunning);
                }
                break;
            }
        }
    }


    void release()
    {
        running = false;
        std::unique_lock<std::mutex> lock(thInfos.mutex_task);
        for (int i = 0; i < num_threads; i++)
        {
            thInfos.set(i, isRunning | isSuspend);
        }
        lock.unlock();
        thInfos.cond.notify_all();

        for (int i = 0; i < num_threads; i++)
        {
            if(pool_[i].joinable()) pool_[i].join();
        }
    }

private:
    int num_threads;
    bool running = true;
    ThreadInfo thInfos;
    std::vector<std::thread> pool_;
};

template<typename CoroutineType,
typename MemoryManager>
class ThreadPoolCoScheduler
{
    typedef std::shared_ptr<MemoryManager> MemoryManagerPtr;
private:
    ThreadPoolCoScheduler() : num_threads(6), mStop(false) {}
    ThreadPoolCoScheduler(int num_th) : num_threads(num_th), mStop(false) {}

public:
    static ThreadPoolCoScheduler& open()
    {
        static ThreadPoolCoScheduler sch;
        return sch;
    }

    template<typename Fn>
    int new_coroutine(Fn&& fn, void* args)
    {
        std::lock_guard<std::mutex> lock(co_pool_lock);
        co_pool_[next_th_idx].new_coroutine(fn, args);
        next_th_idx = (next_th_idx + 1) % num_threads;
    }

    void init()
    {
        co_pool_.reserve(num_threads);
        for (int i = 0; i < num_threads; i++)
        {
            co_pool_.emplace_back();
        }
    }

    void start()
    {
        th_pool_.initPool();
        th_pool_.configure_task(&CoScheduler<CoroutineType, MemoryManager>::global_run);
        for (int i = 0; i < num_threads; i++)
        {
            th_pool_.add_task(&(co_pool_[i]));
        }
    }

    void run()
    {
        th_pool_.wait();
    }

    void close()
    {
        th_pool_.release();
    }

    ~ThreadPoolCoScheduler()
    {
        this->close();
    }

private:
    int num_threads;
    ThreadPool th_pool_;
    std::vector<CoScheduler<CoroutineType, MemoryManager>> co_pool_;
    std::mutex co_pool_lock;
    int next_th_idx = 0;
    bool mStop;
};

#endif
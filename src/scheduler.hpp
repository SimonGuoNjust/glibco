#ifndef __SCHEDULER_H
#define __SCHEDULER_H
#include <thread>
#include "timewheel.hpp"
#include <map>

std::map<std::thread::id, void*> scheduler_store;

using TimerManager=TimeWheel;

template<typename CoroutineType,
typename MemoryManager>
class ThreadPoolCoScheduler;

template<typename CoroutineType,
typename MemoryManager>
class CoScheduler
{
    class CoroutineLink;
    typedef TaskLink<CoroutineLink> coroutine_queue;
    typedef std::shared_ptr<MemoryManager> MemoryManagerPtr;
    friend class ThreadPoolCoScheduler<CoroutineType, MemoryManager>;
    friend class std::vector<CoScheduler<CoroutineType, MemoryManager>>;
private:

    class CoroutineLink
    {

    public:
        CoroutineLink(CoroutineType* const c)
        {
            self = c;
        }

        CoroutineType* get_coroutine()
        {
            return self;
        }

        
        CoroutineLink* pNext;
        CoroutineLink* pPrev;
        coroutine_queue* pLink;
        CoroutineType* self;
    };

public:
    CoScheduler():
    nco(0),
    running(nullptr),
    stack_pool(&MemoryManager::getInstance()),
    pTimeout(new TimerManager(60000)),
    timeout(1)
    {
        coroutines = (coroutine_queue*)calloc(1, sizeof(coroutine_queue) * 3);
    }

    static CoScheduler& getInstance()
    {
        static CoScheduler sch;
        return sch;
    }

    ~CoScheduler()
    {
        this->close();
    }
    

    // static CoScheduler& open()
    // {
    //     static CoScheduler S;
    //     return S;
    // }

    // void close()
    // {
    //     int i;
    //     for (i = 0; i < this->coroutines.size(); i++) {
    //         CoroutineType* co = this->coroutines[i];
    //         if (co) {
    //             co->close();
    //         }
    //     };
    //     delete this->stack_pool;
    //     delete this->pTimeout;
    //     this->stack_pool = nullptr;
    //     this->pTimeout = nullptr;
    // }

    void close()
    {
        int i;
        for (i = 0; i < 3; i++) {
            coroutine_queue* q = coroutines + i;
            CoroutineLink* p = q -> head;
            while(p)
            {
                PopHead<CoroutineLink, coroutine_queue>(coroutines+i);
                p -> self -> close();
                p -> self = nullptr;
                free(p);
                p = q -> head;
            }
        };
        free(coroutines);
        // delete this->stack_pool;
        delete this->pTimeout;
        this->stack_pool = nullptr;
        this->pTimeout = nullptr;
    }

    template<typename Fn>
    int new_coroutine(Fn&& fn, void* args)
    {
        CoroutineType* new_co = coroutine_new(stack_pool, fn, args);
        // int co_id = add_coroutine(new_co);
        add_coroutine(new_co);
    }

    void add_coroutine(CoroutineType* c)
    {
        CoroutineLink& co_task = *((CoroutineLink*)malloc(sizeof(CoroutineLink)));
        memset(&co_task, 0, sizeof(co_task));
        co_task.self = c;
        AddTail(coroutines, &co_task);
        nco++;
    }

    template<typename Fn>
    void add_timeout(Fn&& fn, void* args, size_t timeout_)
    {
        TimerTask& tm = *((TimerTask*)malloc(sizeof(TimerTask)));
        memset(&tm, 0 , sizeof(tm));
        tm.pfnProcess = fn;
        tm.pArg = args;
        unsigned long long now = GetTickMS();
        tm.ullExpireTime = now + timeout_;
        int ret = pTimeout -> addTask(now, &tm);
        if (ret < 0)
        {
            std::cout << "add task error" << std::endl;
            free(&tm);
        }
    }

    static void process_coroutine_timeout(void* co_link)
    {
        CoScheduler* sch = reinterpret_cast<CoScheduler*>(scheduler_store[std::this_thread::get_id()]);
        AddTail(sch->coroutines, reinterpret_cast<CoroutineLink*>(co_link));
    }

    void add_coroutine_timeout(size_t timeout_)
    {
        add_timeout(process_coroutine_timeout, running, timeout_);
        running->self -> yield(WAITING);
    }

    static void global_run(void* objectPtr)
    {
        CoScheduler* this_ = reinterpret_cast<CoScheduler*> (objectPtr);
        auto pid = std::this_thread::get_id();
        if (scheduler_store.find(pid)==scheduler_store.end()) scheduler_store.emplace(pid, objectPtr);
        std::invoke(&CoScheduler::run, this_);
    }

    // long long loop()
    // {
    //     }
    //     return switches;

    // }
    int run_one()
    {
        CoroutineLink* p = coroutines -> head;
        if (p)
        {
            PopHead<CoroutineLink, coroutine_queue>(coroutines);
            running = p;
            p -> self -> resume();
            if (p -> self ->status == ENDED)
            {
                // RemoveFromLink<CoroutineLink, coroutine_queue>(p);
                p -> self ->close();
                p -> self = nullptr;
                nco--;
                free(p);
                return 1;
            }
            else if (p -> self ->status != WAITING)
            {
                AddTail(coroutines, p);
                return 0;
            }
            
        }
        return -1;    
    }

    int process_timeout()
    {
        TimerTaskLink timeout_list;
        TimerTaskLink* ptimeout_list = &timeout_list;
        unsigned long long now = GetTickMS();
        memset(ptimeout_list, 0, sizeof(TimerTaskLink));
        pTimeout->getAllTimeout(now, ptimeout_list);
        TimerTask* p = ptimeout_list->head;
        while(p)
        {
            p -> bTimeout = true;
            p = p->pNext;
        }
        p = ptimeout_list -> head;
        while(p)
        {
            PopHead<TimerTask, TimerTaskLink>(ptimeout_list);
            if (p->pfnProcess)
            {
                pTimeout->do_timeout(p);
            }
            free(p);
            p = ptimeout_list->head;
        }
    }

    void run()
    {
        for(;;)
        {
            process_timeout();
            run_one();
            if (nco <= 0) break;
        }
    }

    long long loop()
    {
        long long switches = static_cast<long long>(0);
        for (;;)
        {
            CoroutineLink* p = coroutines -> head;
            while(p)
            {
                PopHead<CoroutineLink, coroutine_queue>(coroutines);
                p -> self -> resume();
                ++switches;
                if (p -> self ->status == 3)
                {
                    RemoveFromLink<CoroutineLink, coroutine_queue>(p);
                    p -> self ->close();
                    p -> self = nullptr;
                    nco--;
                    free(p);
                }
                else
                {
                    AddTail(coroutines, p);
                }
                p = coroutines -> head;
                // p = ptimeout_list->head;
            }
            if(nco <= 0) break;

        }
        return switches;
    }

    MemoryManager* stack_pool;
    coroutine_queue* coroutines;
    TimerManager* pTimeout;
    int nco;
    CoroutineLink* running;
    int timeout;
};

template<class SchedulerType>
void register_timeout(size_t timeout)
{
    auto pid = std::this_thread::get_id();
    SchedulerType* sch = reinterpret_cast<SchedulerType*>(scheduler_store[pid]);
    sch -> add_coroutine_timeout(timeout);
}

typedef CoScheduler<Coroutine, StackPool> gCoScheduler;

#endif
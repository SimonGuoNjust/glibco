#ifndef __SCHEDULER_H
#define __SCHEDULER_H
#include <thread>
#include "timewheel.hpp"


typedef TimeWheel TimerManager;


template<typename CoroutineType,
typename MemoryManager>
class CoScheduler
{
    class CoroutineLink;
    typedef TaskLink<CoroutineLink> coroutine_queue;
    typedef std::shared_ptr<MemoryManager> MemoryManagerPtr;
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
                delete p;
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
    TimerTask* add_timeout(Fn&& fn, void* args, int timeout)
    {
        TimerTask& tm = *(new TimerTask);
        tm.pfnProcess = fn;
        tm.pArg = args;
        unsigned long long now = GetTickMS();
        tm.ullExpireTime = now + timeout;
        int ret = pTimeout -> addTask(now, &tm);
        if (ret < 0)
        {
            std::cout << "add task error" << std::endl;
            delete &tm;
            return nullptr;
        }
        return &tm;
    }

    void wake_coroutine(CoroutineLink* p)
    {
        if (p -> pLink == coroutines + 1)
        {
            RemoveFromLink<CoroutineLink, coroutine_queue>(p);
        }
        else
        {
            return;
        }
        AddTail(coroutines, p);
    }

    void wake_coroutine(void* p_)
    {
        CoroutineLink* p = reinterpret_cast<CoroutineLink*>(p_);
        wake_coroutine(p);
    }

    template <typename Fn>
    void add_coroutine_timeout(Fn&& fn, size_t timeout_)
    {
        add_timeout(std::forward<Fn>(fn), running, timeout_);
        running->self -> yield(WAITING);
    }

    static void global_run(void* objectPtr)
    {
        CoScheduler* this_ = reinterpret_cast<CoScheduler*> (objectPtr);
        this_->run();
    }

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
                delete p;
                return 1;
            }
            else if (p -> self ->status == WAITING)
            {
                AddTail(coroutines + 1, p);
                return 0;
            }
            else
            {
                AddTail(coroutines, p);
                return 0;
            }
        }
        return -1;    
    }

    void process_timeout()
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
            delete p;
            p = ptimeout_list->head;
        }
    }

    void run()
    {
        for(;;)
        {
            this->process_timeout();
            // if (run_one() == -1) break;
            // if (nco > 1) std::cout << "Thread:" << std::this_thread::get_id() << " nco: " << nco << std::endl;;
            this->run_one();
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
                    delete p;
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

#endif
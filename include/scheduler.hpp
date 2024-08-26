#ifndef __SCHEDULER_H
#define __SCHEDULER_H
#include "timewheel.h"
using TimerManager=TimeWheel;


template<typename CoroutineType>
void OnProcessEvent(CoroutineType* c)
{
    c->resume();
}

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
    running(-1),
    stack_pool(&MemoryManager::getInstance()),
    pTimeout(new TimerManager(60000)),
    timeout(1)
    {
        coroutines = (coroutine_queue*)calloc(1, sizeof(coroutine_queue) * 3);
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
                p -> self -> close();
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
        // TimerTask& tm = *((TimerTask*)malloc(sizeof(TimerTask)));
        // memset(&tm, 0 , sizeof(tm));
        // tm.co_id = co_id;
        // tm.pfnProcess = OnProcessEvent;
        // tm.pArg = nullptr;
        // unsigned long long now = GetTickMS();
        // tm.ullExpireTime = now + timeout;
        // int ret = pTimeout -> addTask(now, &tm);
        // if (ret < 0)
        // {
        //     std::cout << "add task error" << std::endl;
        //     free(&tm);
        // }
        add_coroutine(new_co);
    }

    // int add_coroutine(CoroutineType* c)
    // {
    //     int i;
    //     cap = coroutines.size();
    //     for (i=0;i<cap;i++) {
    //         int id = (i+nco) % cap;
    //         if (coroutines[id] == nullptr) {
    //             coroutines[id] = c;
    //             ++nco;
    //             return id;
    //         }
    //     }
    //     coroutines.push_back(c);
    //     return coroutines.size() - 1;
    // }

    void add_coroutine(CoroutineType* c)
    {
        CoroutineLink& co_task = *((CoroutineLink*)malloc(sizeof(CoroutineLink)));
        memset(&co_task, 0, sizeof(co_task));
        co_task.self = c;
        AddTail(coroutines, &co_task);
    }

    static void global_run(void* objectPtr)
    {
        CoScheduler* this_ = reinterpret_cast<CoScheduler*> (objectPtr);
        std::invoke(&CoScheduler::run, this_);
    }

    // long long loop()
    // {
    //     TimerTaskLink timeout_list;
    //     TimerTaskLink* ptimeout_list = &timeout_list;
    //     long long switches = static_cast<long long>(0);
    //     for (;;)
    //     {
    //         unsigned long long now = GetTickMS();
    //         memset(ptimeout_list, 0, sizeof(TimerTaskLink));
    //         pTimeout->getAllTimeout(now, ptimeout_list);
    //         TimerTask* p = ptimeout_list->head;
    //         while(p)
    //         {
    //             p -> bTimeout = true;
    //             p = p->pNext;
    //         }
    //         p = ptimeout_list -> head;
    //         while(p)
    //         {
    //             PopHead<TimerTask, TimerTaskLink>(ptimeout_list);
    //             if (p->pfnProcess)
    //             {
    //                 p->pfnProcess(coroutines[p->co_id]);
    //                 ++switches;
    //                 // std::cout << p->co_id << std::endl;
    //             }
    //             if (coroutines[p->co_id]->status == 3)
    //             {
    //                 RemoveFromLink<TimerTask, TimerTaskLink>(p);
    //                 coroutines[p->co_id]->close();
    //                 nco--;
    //                 free(p);
    //             }
    //             else
    //             {
    //                 now = GetTickMS();
    //                 p->ullExpireTime = now + timeout;
    //                 pTimeout->addTask(now, p);
    //             }
    //             p = ptimeout_list->head;
    //         }
    //         if(nco <= 0) break;

    //     }
    //     return switches;

    // }
    int run_one()
    {
        CoroutineLink* p = coroutines -> head;
        if (p)
        {
            PopHead<CoroutineLink, coroutine_queue>(coroutines);
            p -> self -> resume();
            if (p -> self ->status == 3)
            {
                RemoveFromLink<CoroutineLink, coroutine_queue>(p);
                p -> self ->close();
                p -> self = nullptr;
                nco--;
                free(p);
                return 1;
            }
            else
            {
                AddTail(coroutines, p);
                return 0;
            }
        }
        return -1;
        
    }

    void run()
    {
        for(;;)
        {
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
    // CoScheduler(const CoScheduler&) = delete;
    // CoScheduler& operator=(const CoScheduler&) = delete;
    // MemoryManager* stack_pool;
    MemoryManager* stack_pool;
    // std::vector<CoroutineType*> coroutines;
    coroutine_queue* coroutines;
    TimerManager* pTimeout;
    int nco;
    int running;
    int timeout;
    bool sharedManager;
};


#endif
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
private:
    CoScheduler():
    nco(0),
    cap(10000),
    running(-1),
    stack_pool(new MemoryManager()),
    pTimeout(new TimerManager(60000)),
    timeout(1)
    {
        this -> coroutines.resize(this->cap);
    }

    ~CoScheduler()
    {
        this->close();
    }

public:
    
    static CoScheduler& open()
    {
        static CoScheduler S;
        return S;
    }

    void close()
    {
        int i;
        for (i = 0; i < this->coroutines.size(); i++) {
            CoroutineType* co = this->coroutines[i];
            if (co) {
                co->close();
            }
        };
        delete this->stack_pool;
        delete this->pTimeout;
        this->stack_pool = nullptr;
        this->pTimeout = nullptr;
    }

    template<typename Fn>
    int new_coroutine(Fn&& fn, void* args)
    {
        CoroutineType* new_co = coroutine_new(stack_pool, fn, args);
        int co_id = add_coroutine(new_co);
        TimerTask& tm = *((TimerTask*)malloc(sizeof(TimerTask)));
        memset(&tm, 0 , sizeof(tm));
        tm.co_id = co_id;
        tm.pfnProcess = OnProcessEvent;
        tm.pArg = nullptr;
        unsigned long long now = GetTickMS();
        tm.ullExpireTime = now + timeout;
        int ret = pTimeout -> addTask(now, &tm);
        if (ret < 0)
        {
            std::cout << "add task error" << std::endl;
            free(&tm);
        }
    }

    int add_coroutine(CoroutineType* c)
    {
        int i;
        cap = coroutines.size();
        for (i=0;i<cap;i++) {
            int id = (i+nco) % cap;
            if (coroutines[id] == nullptr) {
                coroutines[id] = c;
                ++nco;
                return id;
            }
        }
        coroutines.push_back(c);
        return coroutines.size() - 1;
    }

    long long loop()
    {
        TimerTaskLink timeout_list;
        TimerTaskLink* ptimeout_list = &timeout_list;
        long long switches = static_cast<long long>(0);
        for (;;)
        {
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
                    p->pfnProcess(coroutines[p->co_id]);
                    ++switches;
                    // std::cout << p->co_id << std::endl;
                }
                if (coroutines[p->co_id]->status == 3)
                {
                    RemoveFromLink<TimerTask, TimerTaskLink>(p);
                    coroutines[p->co_id]->close();
                    nco--;
                    free(p);
                }
                else
                {
                    now = GetTickMS();
                    p->ullExpireTime = now + timeout;
                    pTimeout->addTask(now, p);
                }
                p = ptimeout_list->head;
            }
            if(nco <= 0) break;

        }
        return switches;

    }
    CoScheduler(const CoScheduler&) = delete;
    CoScheduler& operator=(const CoScheduler&) = delete;
    MemoryManager* stack_pool;
    std::vector<CoroutineType*> coroutines;
    TimerManager* pTimeout;
    int nco;
    int cap;
    int running;
    int timeout;
};


#endif
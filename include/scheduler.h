#ifndef __SCHEDULER_H
#define __SCHEDULER_H
#include <assert.h>
#include <vector>

template<typename CoroutineType,
typename MemoryManager>
class CoScheduler
{
private:
    CoScheduler();

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

    void close();

    template<typename Fn>
    int new_coroutine(Fn&&, void*);
    int add_coroutine(CoroutineType*);
    CoScheduler(const CoScheduler&) = delete;
    CoScheduler& operator=(const CoScheduler&) = delete;
    MemoryManager* stack_pool;
    std::vector<CoroutineType*> coroutines;
    int nco;
    int cap;
    int running;
};


template<typename CoroutineType,
typename MemoryManager>
CoScheduler<CoroutineType, MemoryManager>::CoScheduler()
{
    this -> nco = 0;
    this -> cap = 10000;
    this -> running = -1;
    this -> stack_pool = new MemoryManager();
    this -> coroutines.resize(this->cap);
}

template<typename CoroutineType,
typename MemoryManager>
void CoScheduler<CoroutineType, MemoryManager>::close()
{
    int i;
    for (i = 0; i < this->coroutines.size(); i++) {
        CoroutineType* co = this->coroutines[i];
        if (co) {
            co->close();
        }
    };
    delete this->stack_pool;
    this->stack_pool = nullptr;
}
template<typename CoroutineType, typename MemoryManager>
template<typename Fn> 
int CoScheduler<CoroutineType, MemoryManager>::new_coroutine(Fn&& fn, void* args)
{
    CoroutineType* new_co = coroutine_new(stack_pool, fn, args);
    add_coroutine(new_co);
}
template<typename CoroutineType, typename MemoryManager>
int CoScheduler<CoroutineType, MemoryManager>::add_coroutine(CoroutineType* c)
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


#endif
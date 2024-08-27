#include "gcoroutine.hpp"
#include "scheduler.hpp"

struct args 
{
    CoScheduler* sch;
    Coroutine* co;
};

void timeout_fn(void* arg)
{
    args* pArg = reinterpret_cast<args*>(arg);
    CoScheduler* sch = pArg->sch;
    Coroutine* co = pArg->co;
    
}


int main()
{

}
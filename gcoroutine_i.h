#ifndef __GCO_ROUTINE_I_H
#define __GCO_ROUTINE_I_H
#include <gcoroutine.h>
#include <memorypool.h>
#include <string.h>
#include <assert.h>

struct Coroutine;
struct CoScheduler;



enum {__MAX_COROUTINES=100000};

struct Stack
{
    unsigned int size;
    Coroutine* bind_co;
    void* st_top;
    void* st_bottom;
};

class StackPool
{
public:
    StackPool(char is_shared = 0);
    Stack* get_stack(unsigned int bytes);
    void release_stack(Stack* stack);
    ~StackPool();

protected:
    MemoryPool* memory_manager;
};
void func_wrapper(transfer_t t);

#endif

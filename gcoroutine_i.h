#ifndef __GCO_ROUTINE_I_H
#define __GCO_ROUTINE_I_H
#include <gcoroutine.h>
#include <memorypool.h>
#include <string.h>
#include <assert.h>

struct Coroutine;
struct CoScheduler;

#ifdef USE_BOOST_CONTEXT
typedef fcontext_t CoroutineContext; 
#else
typedef ucontext_t CoroutineContext; 
#endif


enum {__MAX_COROUTINES=100000};

struct Stack
{
    unsigned int size;
    Coroutine* bind_co;
    char* st_top;
    char* st_bottom;
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
#ifdef USE_BOOST_CONTEXT
static void func_wrapper(transfer_t t);
#else
static void func_wrapper(uint32_t low32, uint32_t hi32);
#endif
#endif

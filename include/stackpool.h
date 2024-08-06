#ifndef __STACKPOOL_H
#define __STACKPOOL_H
#include <memorypool.h>
#include <string.h>
#include <assert.h>

enum {__MAX_COROUTINES=100000};

struct Stack
{
    unsigned int size;
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

#endif

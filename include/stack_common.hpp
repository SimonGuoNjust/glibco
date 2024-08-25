#ifndef __STACK_COMMON_H
#define __STACK_COMMON_H
#include <stdexcept>

enum {__DEFAULT_POOLSIZE = 128};
enum {__MINIMUM_BLOCKSIZE = 1024 * 8};

class OutofMemory: public std::runtime_error
{
public:
    OutofMemory(): std::runtime_error("Malloc error") {}
};

class Stack
{
public:
    void* sp;
    size_t size;
    Stack() = default;
    Stack(void* top, size_t size_):
    size(size_)
    {
        sp = reinterpret_cast< void * >(reinterpret_cast< uintptr_t >(top) + 
                                        static_cast< uintptr_t >(size_)); 
    }
    void* get_top()
    {
        return reinterpret_cast< void * >(reinterpret_cast< uintptr_t >(sp) - 
                                        static_cast< uintptr_t >(size));
    }
};
// typedef std::shared_ptr<Stack> StackPtr;
#endif
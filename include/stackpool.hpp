#ifndef __STACKPOOL_H
#define __STACKPOOL_H
// #define __BITMAP_STACKPOOL
#ifdef __BITMAP_STACKPOOL
#include "bitmap_stackpool.hpp"
#else
#include "linker_stackpool.hpp"
#endif

#include "stack_common.hpp"

class StackPool
{
public:
    #ifdef __BITMAP_STACKPOOL
    typedef bitmap_stackpool pool_impl_type;
    #else
    typedef linker_stackpool pool_impl_type;
    #endif

    Stack* get_stack()
    {
        return pool_impl_.get_stack();
    }

    void release_stack(Stack* stack)
    {
        pool_impl_.release_stack(stack);
    }

    ~StackPool()
    {
        delete &pool_impl_;
    }

    static StackPool& getInstance()
    {
        static StackPool sp;
        return sp;
    }

private:
    StackPool():
    pool_impl_(*(new pool_impl_type())) {}

    StackPool(int stack_size): pool_impl_(*(new pool_impl_type(
        stack_size))) {}
    pool_impl_type& pool_impl_;
};

#endif

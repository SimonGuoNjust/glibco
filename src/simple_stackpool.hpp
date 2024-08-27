#ifndef __SIMPLE_STACKPOOL_H
#define __SIMPLE_STACKPOOL_H
#include <string.h>
#include <vector>
#include <assert.h>
#include "stack_common.hpp"

class simple_stackpool
{
public:
    simple_stackpool(){
    }

    simple_stackpool(int){
    }

    Stack* get_stack()
    {
        Stack* new_stack = new Stack();
        new_stack ->size = __MINIMUM_BLOCKSIZE * 8;
        void* m = new_mem(__MINIMUM_BLOCKSIZE * 8);
        new_stack->sp = reinterpret_cast<void*>((reinterpret_cast< char* >(m) + new_stack->size));
        return new_stack;
    }

    void release_stack(Stack* stack)
    {
        void* m = reinterpret_cast<void*>((reinterpret_cast< char* >(stack->sp) - stack->size));
        free(m);
        stack->sp = nullptr;
        stack->size= 0;
        delete stack;
    }

private:
    void* new_mem(ssize_t bytes)
    {
        void* new_ = malloc(bytes);
        if (new_ == nullptr)
        {
            throw OutofMemory();
        }
        return new_;
    }
};

#endif

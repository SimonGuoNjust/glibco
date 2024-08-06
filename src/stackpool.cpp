#include <../include/stackpool.h>

StackPool::StackPool(char is_shared)
{
    if (is_shared == 0)
    {
        memory_manager = new MemoryPool();
    }
}

Stack* StackPool::get_stack(unsigned int bytes)
{
    Stack* new_stack = (Stack*)malloc(sizeof(*new_stack));
    new_stack->size = bytes;
    new_stack->st_top = memory_manager->get_buffer(bytes);
	// new_stack->st_top = malloc(bytes);
    new_stack->st_bottom = static_cast< char * >( new_stack->st_top) + bytes;
    return new_stack;
}

void StackPool::release_stack(Stack* stack)
{
    memory_manager->release_buffer(stack->st_top, stack->size);
	// free(stack->st_top);
	stack->st_top=nullptr;
	stack->st_bottom=nullptr;
	stack->size=0;
}

StackPool::~StackPool()
{
    delete memory_manager;
}

#include <../include/memorypool.h>

void* MemoryPool::get_buffer(unsigned int bytes)
{
    StackMem* volatile *freelist_node;
    StackMem* return_node;
    freelist_node = free_list + get_index(bytes);
    return_node = *freelist_node;
    if (return_node == 0)
    {
        void* r = refill(bytes);
        return r;
    }
    *freelist_node = return_node -> next;
    return (return_node);
}

void* MemoryPool::refill(int n_bytes)
{
    int n_nodes = 10;
    char* new_mem = mem_alloc(n_bytes, n_nodes);
    StackMem* volatile* freelist_node;
    StackMem* return_node;
    StackMem* cur_node, *next_node;
    int i;
    if (n_nodes == 1) return (new_mem);
    freelist_node = free_list + get_index(n_bytes);
    return_node = (StackMem*)new_mem;
    *freelist_node = next_node = (StackMem*)(new_mem + n_bytes);
    for(i = 1; ; i++)
    {
        cur_node = next_node;
        next_node = (StackMem*)((char*)next_node + n_bytes);
        if (n_nodes - 1 == i)
        {
            cur_node->next = NULL;
            break;
        }
        else
        {
            cur_node->next = next_node;
        }
    }
    return return_node;
}

char* MemoryPool::mem_alloc(int size, int n_nodes)
{
    char* return_mem;
    int total_bytes = size * n_nodes;
    int bytes_left = end_free - start_free;
    // std::cout<< size<< " " << n_nodes << " " << total_bytes << " " << bytes_left << std::endl;
    if (bytes_left >= total_bytes) {
        return_mem = start_free;
        start_free += total_bytes;
        return (return_mem);
    }
    else if (bytes_left >= size) {
        n_nodes = bytes_left / size;
        total_bytes = size * n_nodes;
        return_mem = start_free;
        start_free += total_bytes;
        return (return_mem);
    }
    else {
        int bytes_to_get = 2 * total_bytes;
        if (bytes_left > 0)
        {
            StackMem* volatile* freelist_node = free_list + get_index(bytes_left);
            ((StackMem*)start_free) -> next = *freelist_node;
            *freelist_node = (StackMem*)start_free;
        }
        start_free = (char*)malloc(bytes_to_get);
        if (start_free == 0)
        {
            int i;
            StackMem* volatile* freelist_node, *p;
            for (i = size; i <= __MAXIMUM_BLOCKSIZE; i +=__MINIMUM_BLOCKSIZE)
            {
                freelist_node = free_list + get_index(i);
                p = *freelist_node;
                if (p != 0)
                {
                    *freelist_node = p -> next;
                    start_free = (char*)p;
                    end_free = start_free + i;
                    return (mem_alloc(size, n_nodes));
                }
            }
            end_free = 0;
            std::cout << "out of memory" << std::endl;
            exit(1); 
        }
        end_free = start_free + bytes_to_get;
        return (mem_alloc(size, n_nodes));
        
    }
}

void MemoryPool::release_buffer(void* p, unsigned int bytes)
{
    StackMem* q = (StackMem*)p;
    StackMem * volatile* freelist_node;
    freelist_node = free_list + get_index(bytes);
    q -> next = *freelist_node;
    *freelist_node = q;
}

int MemoryPool::get_index(unsigned int bytes)
{
    return (((bytes) + __MINIMUM_BLOCKSIZE - 1) / __MINIMUM_BLOCKSIZE - 1);
}

MemoryPool::StackMem* volatile MemoryPool::free_list[__NSIZELEVEL] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
char* MemoryPool::start_free = 0;
char* MemoryPool::end_free = 0;
unsigned int MemoryPool::heap_size = 0;

MemoryPool::MemoryPool()
{
    start_free = (char*)malloc(__MINIMUM_BLOCKSIZE * __DEFAULT_POOL_SIZE);
    end_free = start_free + __MINIMUM_BLOCKSIZE * __DEFAULT_POOL_SIZE;
}
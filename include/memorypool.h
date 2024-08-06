#include <stdlib.h>
#include <iostream>

enum {__MINIMUM_BLOCKSIZE = 1024 * 8};
enum {__MAXIMUM_BLOCKSIZE = 1024 * 128};
enum {__DEFAULT_POOL_SIZE = 128};
enum {__NSIZELEVEL = __MAXIMUM_BLOCKSIZE / __MINIMUM_BLOCKSIZE};

class MemoryPool
{
protected:
    union StackMem
    {
        char* buffer;
        StackMem* next;
    };
    static StackMem* volatile free_list[__NSIZELEVEL];
    static int get_index(unsigned int bytes);
    static char* mem_alloc(int size, int n_nodes);
    void* refill(int n_bytes);
    static char* start_free;
    static char* end_free;
    static unsigned int heap_size;

public:
    MemoryPool();
    unsigned int pool_size;
    void* get_buffer(unsigned int bytes);
    void release_buffer(void* p, unsigned int bytes);
    
};
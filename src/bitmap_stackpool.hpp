#ifndef __BITMAP_STACKPOOL_H
#define __BITMAP_STACKPOOL_H
#include <string.h>
#include <vector>
#include <assert.h>
#include "stack_common.hpp"

class bitmap_stackpool
{
public:
    bitmap_stackpool(): pool_size_(__DEFAULT_POOLSIZE),
    stack_size_(__MINIMUM_BLOCKSIZE) {
        blocks_.reserve(pool_size_ * 2);
        alloc_bitmap_.reserve(pool_size_ * 2);
        extend_pool(pool_size_);
    }

    bitmap_stackpool(int stack_size): pool_size_(__DEFAULT_POOLSIZE),
    stack_size_(stack_size) {
        blocks_.reserve(pool_size_ * 2);
        alloc_bitmap_.reserve(pool_size_ * 2);
        extend_pool(pool_size_);
    }

    Stack* get_stack()
    {
        Stack* new_stack = bind_chunk();
        while(new_stack == nullptr)
        {
            extend_pool(pool_size_);
            new_stack = bind_chunk();
        };
        return new_stack;
    }

    static bool is_from(void* const chunk, char* const block, const size_t bsz)
    {
        std::less_equal<void*> lt_eq;
        std::less<void*> lt;
        return (lt_eq(block, chunk) && lt(chunk, block + bsz));
    }

    void release_stack(Stack* stack)
    {
        uint32_t n_blocks = alloc_bitmap_.size();
        void* st_top = stack->get_top();
        for (uint32_t blockInd = 0; blockInd < n_blocks; blockInd++)
        {
            if (is_from(st_top, static_cast<char*>(blocks_[blockInd]), 32 * stack_size_))
            {
                uint32_t chunkInd = static_cast<uint32_t>((reinterpret_cast< char* >(stack -> sp) - reinterpret_cast< char* >(blocks_[blockInd]) - stack->size) %
                (32 * stack_size_));
                free_chunk(blockInd, chunkInd);
                break;
            }   
        }
        stack->sp = nullptr;
        stack->size = 0;
        delete stack;
    }

    ~bitmap_stackpool()
    {
        for (int i = 0; i < blocks_.size(); i++)
        {
            free(blocks_[i]);
            blocks_[i] = nullptr;
        }
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

    void extend_pool(int num_blocks)
    {
        size_t required_bytes = num_blocks * stack_size_ * 32;
        void* mem = new_mem(required_bytes);

        for (int i = 0; i < num_blocks; i++)
        {
            blocks_.push_back(reinterpret_cast<void*>(reinterpret_cast< char* >(mem) + 
        (i * stack_size_ * 32)));
            alloc_bitmap_.push_back(0); 
        }
    }

    inline bool is_block_free(uint32_t blockInd)
    {
        return alloc_bitmap_[blockInd] == 0;
    }

    inline bool is_block_full(uint32_t blockInd)
    {
        return alloc_bitmap_[blockInd] == ~static_cast<uint32_t>(0x0);
    }

    inline bool is_chunk_free(uint32_t blockInd, uint32_t chunkInd)
    {
        return (alloc_bitmap_[blockInd] & (1 << chunkInd)) == 0;
    }

    inline void set_chunk(uint32_t blockInd, uint32_t chunkInd)
    {
        alloc_bitmap_[blockInd] |= (1 << chunkInd);
    }

    inline void free_chunk(uint32_t blockInd, uint32_t chunkInd)
    {
        alloc_bitmap_[blockInd] &= ~(1 << chunkInd);
    }

    inline void* get_chunk(uint32_t blockInd, uint32_t chunkInd)
    {
        return reinterpret_cast<void*>(reinterpret_cast< char* >(blocks_[blockInd]) + 
        chunkInd * stack_size_); 
    }

    Stack* bind_chunk()
    {
        uint32_t blockInd = 0;
        uint32_t n_blocks = alloc_bitmap_.size();
        for (blockInd = 0; blockInd < n_blocks; blockInd++)
        {
            if (is_block_full(blockInd)) continue;
            for (uint32_t chunkInd = 0; chunkInd < 32; chunkInd++)
            {
                if (is_chunk_free(blockInd, chunkInd))
                {
                    set_chunk(blockInd, chunkInd);
                    void* top = get_chunk(blockInd, chunkInd);
                    return new Stack(top, stack_size_);
                }
            }
        }
        return nullptr;
    }

    size_t pool_size_;
    size_t stack_size_;
    std::vector<void*> blocks_;
    std::vector<uint32_t> alloc_bitmap_;
};

#endif

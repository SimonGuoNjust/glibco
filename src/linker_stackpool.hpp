#ifndef __LINKER_STACKPOOL_H
#define __LINKER_STACKPOOL_H
#include <string.h>
#include <assert.h>
#include "stack_common.hpp"

class linker_stackpool
{
public:
    linker_stackpool(): pool_size_(__DEFAULT_POOLSIZE),
    stack_size_(__MINIMUM_BLOCKSIZE),
    chunk_list(0) {
        extend_pool(pool_size_ * stack_size_ * 2);
    }

    linker_stackpool(int stack_size): pool_size_(__DEFAULT_POOLSIZE),
    stack_size_(stack_size),
    chunk_list(0) {
        extend_pool(pool_size_ * stack_size_ * 2);
    }

    Stack* get_stack()
    {
        if (chunk_list == nullptr)
        {
            extend_pool(pool_size_ * 32 * __MINIMUM_BLOCKSIZE);
        }
        Stack* const ret = &(chunk_list -> st);
        chunk_list = chunk_list -> nextptr;
        return ret;
    }

    void release_stack(Stack* stack)
    {
        StackNode* ret = reinterpret_cast<StackNode*>(reinterpret_cast<char*>(stack) - sizeof(StackNode*));
        ret->nextptr = chunk_list;
        chunk_list = ret;
    }

    ~linker_stackpool()
    {
        purge_mem();
    }

private:

    class StackNode
    {
    public:
        StackNode* nextptr;
        Stack st;
        StackNode(void* ptr, char* top, size_t size):
        st(reinterpret_cast<void*>(top), size)
        {
            nextptr = reinterpret_cast<StackNode*>(ptr);
        }
    };

    class BlockNode
    {
    private:
        char* startptr;
        size_t size;
        char * get_next_size_addr() const
        {
        return (startptr + size - sizeof(size_t));
        }
        char* get_next_ptr_addr() const
        {
            return (startptr + size - sizeof(BlockNode));
        }
    public:

        BlockNode():
        startptr(0),
        size(0)
        {
        }
        BlockNode(char * const sptr, size_t size_):
        startptr(sptr),
        size(size_)
        {
        }
        char* begin() const
        {
            return startptr;
        }

        char* end() const
        {
            return get_next_ptr_addr();
        }

        char*& begin()
        {
            return startptr;
        }

        size_t total_size() const
        {
            return size;
        }

        size_t actual_size() const
        {
            return static_cast<size_t>(size - sizeof(BlockNode));
        }

        BlockNode* get_next_node_ptr() const
        {
            return static_cast<BlockNode*>(static_cast<void*>(get_next_ptr_addr()));
        }

        size_t& get_next_size() const 
        {
            return get_next_node_ptr() -> size;
        }

        char* & get_next_startptr() const 
        {
            return get_next_node_ptr() -> startptr;
        }

        BlockNode& set_next(char* const startptr_, size_t size_) const
        {
            return *(new (get_next_ptr_addr()) BlockNode(startptr_, size_));
        }

        BlockNode& get_next() const
        {
             return *(reinterpret_cast<BlockNode*>(get_next_ptr_addr()));
        }   

    };

    bool purge_mem()
    {
        BlockNode iter = block_list;
        if (iter.begin() == 0)
        {
            return false;
        }

        do
        {
            const BlockNode next = iter.get_next();
            free(iter.begin());
            iter = next;

        } while (iter.begin() != 0);

        block_list.begin() = 0;
        this->chunk_list = 0;
        return true;
    }

    void* new_mem(ssize_t bytes)
    {
        void* new_ = malloc(bytes);
        if (new_ == nullptr)
        {
            throw OutofMemory();
        }
        return new_;
    }

    inline StackNode* set_stacknode(char* addr, void* nextpr, size_t stack_size)
    {
        return new (reinterpret_cast<void*>(addr)) StackNode{nextpr, addr + sizeof(StackNode*), stack_size - sizeof(StackNode*)};
    }

    static bool is_from(void* const chunk, char* const block, const size_t bsz)
    {
        std::less_equal<void*> lt_eq;
        std::less<void*> lt;
        return (lt_eq(block, chunk) && lt(chunk, block + bsz));
    }

    void* segregate(
        void* const block,
        const size_t sz,
        const size_t stack_sz,
        void* const end
    )
    {
        char* final_ = reinterpret_cast<char*>(block) + ((sz - stack_sz) / stack_sz) * stack_sz;
        set_stacknode(final_, end, stack_sz);
        if (final_ == block) return block;
        for (char* it_ = final_ - stack_sz; it_ != block; final_ = it_, it_ -= stack_sz)
        {
            set_stacknode(it_, final_, stack_sz);
        }
        set_stacknode(static_cast<char*>(block), final_, stack_sz);
        return block;
    }

    void extend_pool(size_t bytes)
    {
        char* mem = static_cast<char*>(new_mem(bytes));
        BlockNode node (mem, bytes);
        chunk_list = reinterpret_cast<StackNode*>(segregate(mem, node.actual_size(), stack_size_, chunk_list));
        node.set_next(block_list.begin(), block_list.total_size());
        block_list = node;
    }

    unsigned int pool_size_;
    unsigned int stack_size_;
    BlockNode block_list;
    StackNode* chunk_list;
    
};

#endif

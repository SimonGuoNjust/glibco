#include <boost/context/all.hpp>
#include <iostream>

using namespace boost::context::detail;
using co_func_type = void(*)(struct co*, void*);
int id = 0;
enum co_status{
    FINISHED=0,
    SUSPEND,
    RUNNING,
};

struct co
{
    co_func_type func;
    void* args;
    fcontext_t self_ctx;
    fcontext_t main_ctx;
    co_status status;
};

struct jump_data
{
    co* from_co;
};

// struct sch
// {
//     int running;
//     std::vector<co*> cors;
// };

void foo(co* co_, void* args)
{
    int id = *reinterpret_cast<int*>(args);
    int a = 150;
    while(a--)
    { 
        std::cout << "coroutine " << id << ":" << a << std::endl;
        co_->status = SUSPEND;
        transfer_t t = jump_fcontext(co_->main_ctx, nullptr);
    }
}

void func_wrapper(transfer_t src_ctx)
{
    jump_data* jump_src = reinterpret_cast<jump_data*>(src_ctx.data);
    jump_src->from_co->main_ctx = src_ctx.fctx;
    jump_src->from_co->status =RUNNING;
    foo(jump_src->from_co, jump_src->from_co->args);
    jump_src->from_co->status =FINISHED;
    jump_fcontext(src_ctx.fctx, nullptr);
}

int main()
{
    // char st1[1024 * 8];
    // char st2[1024 * 8];
    void* st1 = malloc(1024 * 32);
    void* st2 = malloc(1024 * 32);
    int id1 = 1;
    int id2 = 2;
    fcontext_t co_ctx_1 = make_fcontext(st1, 1024 * 32, func_wrapper);
    fcontext_t co_ctx_2 = make_fcontext(st2, 1024 * 32, func_wrapper);
    co* co1 = new co;
    co* co2 = new co;
    co1->func = foo;
    co2->func = foo;
    co1->status = SUSPEND;
    co2->status = SUSPEND;
    co1->args = &id1;
    co2->args = &id2;
    co1->self_ctx = co_ctx_1;
    co2->self_ctx = co_ctx_2;
    jump_data* jmp_data1 = new jump_data;
    jmp_data1->from_co = co1;
    jump_data* jmp_data2 = new jump_data;
    jmp_data2->from_co = co2;

    transfer_t t;
    while(1)
    {
        if (co1->status!=FINISHED)
        {
            t = jump_fcontext(co1->self_ctx, jmp_data1);
            co1->self_ctx = t.fctx;
            std::cout << "return to main" << std::endl;
        }
        if (co2->status != FINISHED)
        {
            t = jump_fcontext(co2->self_ctx, jmp_data2);
            co2->self_ctx = t.fctx;
            std::cout << "return to main" << std::endl;
        }
        if (co1->status == FINISHED && co2->status == FINISHED)
        {
            break;
        }
    }
    std::cout << "main end" << std::endl;
    return 0;
}
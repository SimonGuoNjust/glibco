#include <boost/context/all.hpp>
#include <iostream>

using namespace boost::context::detail;
int id = 0;
enum co_status{
    FINISHED=0,
    SUSPEND,
    RUNNING,
};

struct co
{
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

void foo(co* co_, int id)
{
    int a = 3;
    while(a--)
    { 
        std::cout << "coroutine " << id << ":" << a << std::endl;
        co_->status = SUSPEND;
        jump_fcontext(co_->main_ctx, nullptr);
    }
}

void func_wrapper(transfer_t src_ctx)
{
    jump_data* jump_src = reinterpret_cast<jump_data*>(src_ctx.data);
    jump_src->from_co->main_ctx = src_ctx.fctx;
    jump_src->from_co->status =RUNNING;
    foo(jump_src->from_co, id++);
    jump_src->from_co->status =FINISHED;
    jump_fcontext(src_ctx.fctx, nullptr);
}

int main()
{
    // char st1[1024 * 8];
    // char st2[1024 * 8];
    void* st1 = malloc(1024 * 32);
    void* st2 = malloc(1024 * 32);
    fcontext_t co_ctx_1 = make_fcontext(st1, 1024 * 32, func_wrapper);
    fcontext_t co_ctx_2 = make_fcontext(st2, 1024 * 32, func_wrapper);
    co co1, co2;
    co1.self_ctx = co_ctx_1;
    co2.self_ctx = co_ctx_2;
    jump_data* jmp_data1 = new jump_data;
    jmp_data1->from_co = &co1;
    jump_data* jmp_data2 = new jump_data;
    jmp_data2->from_co = &co2;

    transfer_t t;
    while(1)
    {
        if (co1.status!=FINISHED)
        {
            t = jump_fcontext(co1.self_ctx, jmp_data1);
            co1.self_ctx = t.fctx;
            std::cout << "return to main" << std::endl;
        }
        if (co2.status != FINISHED)
        {
            t = jump_fcontext(co2.self_ctx, jmp_data2);
            co2.self_ctx = t.fctx;
            std::cout << "return to main" << std::endl;
        }
        if (co1.status == FINISHED && co2.status == FINISHED)
        {
            break;
        }
    }
    std::cout << "main end" << std::endl;
    return 0;
}
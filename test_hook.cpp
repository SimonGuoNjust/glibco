#include <sys/socket.h>
// #include "include/hook.h"
// // int socket(int __domain, int __type, int __protocol);
// typedef int (*socket_pfn_t)(int domain, int type, int protocol);
// int main()
// {
//     SysCallHook<socket_pfn_t> h;
//     h.hook("socket", my_socket);
    
//     int fd = socket(AF_INET, SOCK_STREAM, 0);
//     return 0;
// }

#include<stdio.h>
#include<stdlib.h>
#include<capstone/capstone.h>
#include<unistd.h>
#include<sys/types.h>
#include<dlfcn.h>
#include<sys/mman.h>
#include<string.h>
#include<stdint.h>
#include<iostream>

extern "C" int hooked(int, int, int);
 //使用mmap来分配一个内存，用于后续的一级跳板
void  *tempAddr = (void  *)0x555555554000;
uint64_t func_wrapper;
//替换目标函数的前len个字节，使之跳转到hook函数
void change_bytes(intptr_t addr, const char code[], int len)
{
    memcpy((void*)addr, code, len);
}
 
 
//获取目标函数前几条指令的长度,跳转指令是5个字节，所以取大于等于5的长度
int get_asm_len(intptr_t target)
{
    csh handle;
    cs_insn* insn;
    size_t count;
    char code[30] = {0};
    int rv;
 
    memcpy((void*)code, (void*)target, 30);
    if(cs_open(CS_ARCH_X86, CS_MODE_64, &handle))
    {
        printf("Error: cs_open\n");
        return -1;
    }
 
    count = cs_disasm(handle, (uint8_t*)code, 30, 0, 0, &insn);
    if(count)
    {
        for(size_t i = 0; i < count; i++)
        {
        //
            if(!strcmp("call", insn[i].mnemonic))
                return 0;
            if (insn[i].address >= 15)
            {
                rv = insn[i].address;
                break;
            }
        }
        cs_free(insn, count);
    }
    else
    {
        printf("Error: cs_disasm\n");
        return -1;
    }
    cs_close(&handle);
    return rv;
}
 

//根据当前temp地址重新填写相对地址
int change_asm_code(intptr_t target ,intptr_t temp)
{
    csh handle;
    cs_insn* insn;
    size_t count;
    uint8_t code[30] = {0};
    int rv;
 
    memcpy((void*)code, (void*)target, 30);
    if(cs_open(CS_ARCH_X86, CS_MODE_64, &handle))
    {
        printf("Error: cs_open\n");
        return -1;
    }
 
    count = cs_disasm(handle, code, 30, 0, 0, &insn);
    if(count)
    {
        for(size_t i = 0; i < count; i++)
        {
        //
            if(!strcmp("call", insn[i].mnemonic))
            {    
                int at = insn[i].address;
                uint32_t originAddr = code[at + 4]<<24|code[at + 3]<<16|code[at + 2]|code[at + 1];
                uint32_t targetAddr = target + at + originAddr + 5;
                uint32_t relativeAddr = temp - targetAddr - 5;
                uint8_t jumpCode[5] = {0xe8,0x00,0x00,0x00,0x00};
                memcpy(jumpCode + 1, &relativeAddr, sizeof(uint32_t));
                change_bytes(targetAddr,(const char*) jumpCode, 5);    
            }
            if (insn[i].address >= 5)
            {
                rv = insn[i].address;
                break;
            }
        }
        cs_free(insn, count);
    }
    else
    {
        printf("Error: cs_disasm\n");
        return -1;
    }
    cs_close(&handle);
    return rv;
}

 
void func_hook(intptr_t target_addr, void* hook_func)
{
    
    //根据目标函数的地址确定目标函数所在的页，并将该页的权限改为可读可写可执行
    intptr_t page_start = target_addr & 0xfffffffff000;
    mprotect((void*)page_start, 0x1000, PROT_READ|PROT_WRITE|PROT_EXEC);
 
 
    int asm_len = get_asm_len(target_addr);
    if(asm_len == 0)
    {
        change_asm_code(target_addr, func_wrapper + 13 + 14);
    }
    if(asm_len == -1)
    {
        printf("Error: get_asm_code\n");
        exit(-1);
    }
    
    
    //保存跳板中的地址到rsp中，这样自定义函数返回时可以返回跳板函数继续执行原函数
    intptr_t x = (intptr_t)((char*)func_wrapper + 27);
    uint8_t ret_code[13] = {(uint8_t)0x68,
    (x&0xff),
    (x&0xff00)>>8,
    (x&0xff0000)>>16,
    (x&0xff000000)>>24,
    (uint8_t)0xC7,
    (uint8_t)0x44,
    (uint8_t)0x24,
    (uint8_t)0x04,
    (x&0xff00000000)>>32,
    (x&0xff0000000000)>>40,
    (x&0xff000000000000)>>48,
    (x&0xff00000000000000)>>56};
    memcpy((void*)func_wrapper, (void*)ret_code, 13);   
    // 7ffff7ffa01b
    
    //跳板地址跳到自定义函数
    // 555555555afd
    intptr_t y = (intptr_t)hook_func;
    uint8_t self_code[14] = {(uint8_t)0x68,
    y&0xff,
    (y&0xff00)>>8,
    (y&0xff0000)>>16,
    (y&0xff000000)>>24,
    (uint8_t)0xC7,
    (uint8_t)0x44,
    (uint8_t)0x24, 
    (uint8_t)0x04,
    (y&0xff00000000)>>32,
    (y&0xff0000000000)>>40,
    (y&0xff000000000000)>>48,
    (y&0xff00000000000000)>>56,
    (uint8_t)0xC3};
    memcpy((void*)((char*)func_wrapper + 13), (void*)self_code, 14);   
    
    //复制原始指令
    memcpy((void*)((char*)func_wrapper+13+14), (void*)target_addr, asm_len);
    
    //跳转指令，跳回原始地址+asmlen处
    intptr_t z = (intptr_t)((char*)target_addr + asm_len);
    //构造push&ret跳转，填入目标地址
    uint8_t jmp_code[14] = {(uint8_t)0x68,
    z&0xff,
    (z&0xff00)>>8,
    (z&0xff0000)>>16,
    (z&0xff000000)>>24,
    (uint8_t)0xC7,
    (uint8_t)0x44,
    (uint8_t)0x24,
    (uint8_t)0x04,
    (z&0xff00000000)>>32,
    (z&0xff0000000000)>>40,
    (z&0xff000000000000)>>48,
    (z&0xff00000000000000)>>56,
    (uint8_t)0xC3};    
    memcpy((void*)((char*)func_wrapper+asm_len+13+14), (void*)jmp_code, 14);
        
    //目标地址改为跳到跳板地址
    // uint64_t relativeAddr = (uint64_t)target_addr - (uint64_t)func_wrapper - 9;
    intptr_t fn = (intptr_t)func_wrapper;
    uint8_t jumpCode[14] = {(uint8_t)0x68,
    fn&0xff,
    (fn&0xff00)>>8,
    (fn&0xff0000)>>16,
    (fn&0xff000000)>>24,
    (uint8_t)0xC7,
    (uint8_t)0x44,
    (uint8_t)0x24,
    (uint8_t)0x04,
    (fn&0xff00000000)>>32,
    (fn&0xff0000000000)>>40,
    (fn&0xff000000000000)>>48,
    (fn&0xff00000000000000)>>56,
    (uint8_t)0xC3};    
    // memcpy(jumpCode + 1, &relativeAddr, sizeof(uint64_t));
    change_bytes(target_addr, (const char*) jumpCode, 14);       
    
     //用于后续的hook函数使用
    func_wrapper = func_wrapper + 60;
    
     
    
}
 
 
 
//
int test_hook(int a, int b, int c)
{
    __asm(
        "push %rbp;"
        "push %rdi;"
        "push %rsi;"
        "push %rdx;"
        "push %rcx;"
        // "push %rax;"
        );
    /*__asm{
        push rbp;
        push rdi;
        push rsi;
        push rdx;
        push rcx;
        push rax;
    }*/
    // printf("everythint is ok\n");
    // register int *rax __asm ("%rax"); 
    // std::cout << *rax << std::endl;
    // printf("rax is %lx\n",rax);
    std::cout << "hooked" << std::endl;
    std::cout << a << std::endl;
    
    asm(
        // "pop %rax;"
        "pop %rcx;"
        "pop %rdx;"
        "pop %rsi;"
        "pop %rdi;"
        "pop %rbp;"
    );
    
}
 
//so被加载后会首先执行这里的代码
__attribute__((constructor))
void load()
{
    func_wrapper = (uintptr_t)mmap(tempAddr, 4096, PROT_WRITE|PROT_EXEC|PROT_READ, MAP_ANON|MAP_PRIVATE, -1, 0);
    func_hook((uint64_t)dlsym(RTLD_DEFAULT, "socket"), (void*)test_hook);
 
    printf("inject suceessfully\n");
}

int main()
{
    int a = socket(AF_INET, SOCK_STREAM, 0);
    std::cout << a << std::endl;
    // test_hook();
    // while(1);
    return 0;
}
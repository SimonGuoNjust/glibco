#define GNU_SOURCE
#include <iostream>
#include <dlfcn.h>
#include <string.h>
#include <functional>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/mman.h>
#define ADDRESS_BYTES 8

intptr_t get_libc_base(pid_t pid)
{
    intptr_t libcAddr;
    char* buf;
    char* end;
    FILE* fd = fopen("/proc/self/maps", "r");
    if(!fd)
    {
        printf("open maps error!");
        exit(1);
    }
    buf = (char*) malloc(0x100);
    do{
        fgets(buf, 0x100, fd);
    } while(!strstr(buf, "libc-"));
    end = strchr(buf, '-');
    libcAddr = strtol(buf, &end, 16);
    // printf("The process %d's libcbase is: 0x%lx\n", pid, libcAddr);
    fclose(fd);
    return libcAddr;
}

//获取目标函数的地址
intptr_t get_target_addr(intptr_t libc_base, const char* func_name)
{
    intptr_t funcAddr;
 
    funcAddr = (size_t)dlsym(0, func_name);
    funcAddr -= get_libc_base(getpid());
    funcAddr += libc_base;
    // printf("function %s address is: 0x%lx\n", func_name, funcAddr);
    return funcAddr;
}

typedef int (*socket_pfn_t)(int domain, int type, int protocol);

int my_socket(int __domain, int __type, int __protocol)
{
    // int (*origin_socket)(int, int, int) = (int(*)(int, int, int))dlsym(RTLD_NEXT, "socket");
    std::cout << "hooked!" << std::endl;

    return -1;
}

template<typename Fn>
class SysCallHook
{
private:
    Fn m_n_call_address = nullptr;
    Fn m_o_call_address = nullptr;
    char m_old_bytes[ADDRESS_BYTES + 1];
    char m_jump_bytes[ADDRESS_BYTES + 1];
public:
    SysCallHook()
    {
        memset(m_old_bytes, 0, ADDRESS_BYTES + 1);
        memset(m_jump_bytes, 0, ADDRESS_BYTES + 1);
    }

    bool hook(const char* name, Fn&& fn)
    {
        m_n_call_address = fn;
        if (m_o_call_address != nullptr)
        {
            unhook();
        }
        // auto libc_base = get_libc_base(getpid());
        // m_o_call_address = (Fn)get_target_addr(libc_base, name);
        m_o_call_address = (Fn)dlsym(RTLD_DEFAULT, name);
        if (m_o_call_address == nullptr)
        {
            return false;
        }
        memcpy(m_old_bytes, (char*)m_o_call_address, ADDRESS_BYTES + 1);
        m_jump_bytes[0] = '\xE9';
        // auto a = sizeof(fn);
        *(uint64_t*)(m_jump_bytes + 1) = (uint64_t)fn - (uint64_t)m_o_call_address - (ADDRESS_BYTES + 1);
        // auto cr0 = clear_and_return_cr0();
        intptr_t page_start = (uintptr_t)m_o_call_address & 0xfffffffff000;
        mprotect((void*)page_start, 0x1000, PROT_READ|PROT_WRITE|PROT_EXEC);
        memcpy((char*)m_o_call_address, m_jump_bytes, ADDRESS_BYTES + 1);
        // setback_cr0(cr0);
        return true;
    }

    inline void rehook()
    {
        memcpy((char*)m_o_call_address, m_jump_bytes, ADDRESS_BYTES + 1);
    }

    inline bool unhook()
    {
        if (m_o_call_address != nullptr)
        {
            memcpy((char*)m_o_call_address, m_old_bytes, ADDRESS_BYTES + 1);
            m_o_call_address = nullptr;
        }
    }
    
};
#define GNU_SOURCE
#include <iostream>
#include <dlfcn.h>
#include <string.h>
#include <functional>
#include <sys/socket.h>
extern "C"
{
    int hooked(int a, int b, int c)
    {
        std::cout << "h" << std::endl;
        return 0;
    }
}


typedef int (*socket_pfn_t)(int domain, int type, int protocol);

int my_socket(int __domain, int __type, int __protocol)
{
    // int (*origin_socket)(int, int, int) = (int(*)(int, int, int))dlsym(RTLD_NEXT, "socket");
    socket_pfn_t origin_socket = &socket;
    std::cout << "hooked!" << std::endl;
    if (origin_socket)
    {
        return origin_socket(__domain, __type, __protocol);
    }
    return -1;
}

class SysCallHook
{
private:
    void* m_n_call_address = nullptr;
    void* m_o_call_address = nullptr;
    char m_old_bytes[5];
    char m_jump_bytes[5];
public:
    SysCallHook()
    {
        memset(m_old_bytes, 0, 5);
        memset(m_jump_bytes, 0, 5);
    }
    template<typename Fn>
    bool hook(const char* name, Fn&& fn)
    {
        m_n_call_address = &fn;
        if (m_o_call_address != nullptr)
        {
            unhook();
        }

        m_o_call_address = dlsym(RTLD_DEFAULT, name);
        if (m_o_call_address == nullptr)
        {
            return false;
        }
        memcpy(m_old_bytes, (char*)m_o_call_address, 5);
        m_jump_bytes[0] = '\xE9';
        *(uint32_t*)(m_jump_bytes + 1) = (uint32_t)&fn - (uint32_t)m_o_call_address - 5;
        memcpy((char*)m_o_call_address, m_jump_bytes, 5);
        return true;
    }

    inline void rehook()
    {
        memcpy((char*)m_o_call_address, m_jump_bytes, 5);
    }

    inline bool unhook()
    {
        if (m_o_call_address != nullptr)
        {
            memcpy((char*)m_o_call_address, m_old_bytes, 5);
            m_o_call_address = nullptr;
        }
    }
    
};
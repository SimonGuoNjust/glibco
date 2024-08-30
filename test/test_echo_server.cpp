#include<stdio.h>
#include<iostream>
#include<cstring>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<errno.h>
#include<sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../src/glibco_all.hpp"
using namespace std;

static void SetAddr(const char *pszIP,const unsigned short shPort,struct sockaddr_in &addr)
{
	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(shPort);
	int nIP = 0;
	if( !pszIP || '\0' == *pszIP   
	    || 0 == strcmp(pszIP,"0") || 0 == strcmp(pszIP,"0.0.0.0") 
		|| 0 == strcmp(pszIP,"*") 
	  )
	{
		nIP = htonl(INADDR_ANY);
	}
	else
	{
		nIP = inet_addr(pszIP);
	}
	addr.sin_addr.s_addr = nIP;
}


struct task
{
    int fd;
};

int glibco_accept(int fd, struct sockaddr *addr, socklen_t *len );

static int g_listen_fd = -1;
static int SetNonBlock(int iSock)
{
    int iFlags;

    iFlags = fcntl(iSock, F_GETFL, 0);
    iFlags |= O_NONBLOCK;
    iFlags |= O_NDELAY;
    int ret = fcntl(iSock, F_SETFL, iFlags);
    return ret;
}

static void readwrite_func(void* arg)
{
    task* t = reinterpret_cast<task*>(arg);
    int fd = t->fd;
    char buf[1024];
    for (;;)
    {
        int ret = read(fd, buf, sizeof(buf));
        if (ret > 0)
        {
            ret = write(fd, buf, ret);
        }
        if( ret <= 0 )
        {
            if (errno == EAGAIN)
                continue;
            close(fd);
            break;
        }
    }
    delete t;
};

static void accept_func(void*)
{
    for (;;)
    {
        struct sockaddr_in addr; //maybe sockaddr_un;
		memset( &addr,0,sizeof(addr) );
		socklen_t len = sizeof(addr);
		// accept
		int fd = glibco_accept(g_listen_fd, (struct sockaddr *)&addr, &len);
		if( fd < 0 )
		{
			// 意思是，如果accept失败了，没办法，暂时切出去
            register_timeout<EpollScheduler>(1000);
			continue;
		}
        else
        {
            task* t = new task;
            t->fd = fd;
            EpollScheduler::open().new_coroutine(readwrite_func, t);
        }
    }
}


static int CreateTcpSocket(const unsigned short shPort /* = 0 */,const char *pszIP /* = "*" */,bool bReuse /* = false */)
{
	int fd = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
	if( fd >= 0 )
	{
		if(shPort != 0)
		{
			if(bReuse)
			{
				int nReuseAddr = 1;
				setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&nReuseAddr,sizeof(nReuseAddr));
			}
			struct sockaddr_in addr ;
			SetAddr(pszIP,shPort,addr);
			int ret = bind(fd,(struct sockaddr*)&addr,sizeof(addr));
			if( ret != 0)
			{
				close(fd);
				return -1;
			}
		}
	}
	return fd;
}

int main(int argc,char *argv[])
{
    
    // first step ->create socket for server
    if (argc <= 2) return -1; 
    const char *ip = argv[1];
	int port = atoi( argv[2] );

    g_listen_fd = CreateTcpSocket( port,ip,true );
    listen( g_listen_fd,1024 );

    if(g_listen_fd==-1){
		printf("Port %d is in use\n", port);
		return -1;
	}
	printf("listen %d %s:%d\n",g_listen_fd,ip,port);

    SetNonBlock( g_listen_fd );

    pid_t pid;
    pid = fork();
    if (pid == 0)
    {
        EpollScheduler& S = EpollScheduler::open();
	    S.start();
        S.new_coroutine(accept_func, nullptr);
        S.run();
        S.close();
    }
    else
    {
        EpollScheduler& cli_S = EpollScheduler::open();
	    cli_S.start();
        int cli_fd = socket(PF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        SetAddr(ip, port, addr);
        
        int ret = connect(cli_fd,(struct sockaddr*)&addr,sizeof(addr));
        if ( errno == EALREADY || errno == EINPROGRESS )
		{
            sleep(1);
            int error = 0;
            uint32_t socklen = sizeof(error);
            errno = 0;
            ret = getsockopt(cli_fd, SOL_SOCKET, SO_ERROR,(void *)&error,  &socklen);
            printf("connect susccess");
        }
    }
    

    // }

    return 0;
    
}
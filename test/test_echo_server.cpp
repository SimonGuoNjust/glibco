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
#include <queue>
#include <shared_mutex>
#include "../src/glibco_all.hpp"
using namespace std;

queue<int> fds;

struct args
{
    queue<int>* pfds;
    mutex fds_lock;
};

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
    args* pArgs = reinterpret_cast<args*>(arg);

    int fd = -1;
    while(1)
    {
        unique_lock<mutex> lk(pArgs->fds_lock);
        queue<int>& fds_ = *(pArgs->pfds);
        if (fds_.empty())
        {
            lk.unlock();
            register_timeout<EpollScheduler>(1000);
        }
        else
        {
            fd = pArgs->pfds->front();
            pArgs->pfds->pop();
            lk.unlock();
            break;
        }
    }

    printf("processing fd : %d \n", fd);

    char buf[512];
    for (;;)
    {
        int ret = read(fd, buf, sizeof(buf));
        if (ret > 0)
        {
            printf("echo %s fd %d\n size %d ", buf,fd, ret);
            ret = write(fd, buf, ret);
        }
        if( ret <= 0 )
        {
            printf("echo errno %d (%s)\n", errno, strerror(errno));
            if (errno == EAGAIN)
                continue;
            close(fd);
            break;
        }
    }
};

static void accept_func(void* arg)
{
    args* pArgs = reinterpret_cast<args*>(arg);

    for (;;)
    {
        struct sockaddr_in addr; //maybe sockaddr_un;
		memset( &addr,0,sizeof(addr) );
		socklen_t len = sizeof(addr);
		// accept
        // printf("try accept at tcp s : %d \n", g_listen_fd);
		int fd = glibco_accept(g_listen_fd, (struct sockaddr *)&addr, &len);
		if( fd < 0 )
		{
			// 意思是，如果accept失败了，没办法，暂时切出去
            printf("waiting connect \n");
            register_timeout<EpollScheduler>(1000);
			continue;
		}

        printf("One connected %d \n", fd);
        // {
        //     printf("push %d", fd);
        {lock_guard<mutex> lk(pArgs->fds_lock);
        pArgs->pfds->push(fd);}
        // }
        register_timeout<EpollScheduler>(10);
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
    if (argc <= 3) return -1; 
    const char *ip = argv[1];
	int port = atoi( argv[2] );
    // int num_thread = atoi( argv[3] );
    int num_cos = atoi( argv[3] );

    g_listen_fd = CreateTcpSocket( port,ip,true );
    listen( g_listen_fd,1024 );

    if(g_listen_fd==-1){
		printf("Port %d is in use\n", port);
		return -1;
	}
	printf("listen %d %s:%d\n",g_listen_fd,ip,port);

    SetNonBlock( g_listen_fd );

    EpollScheduler& S = EpollScheduler::open();
    S.start();

    args* readwrite_arg = new args;
    readwrite_arg->pfds = &fds;

    S.new_coroutine(accept_func, readwrite_arg);

    for (int i = 0; i < num_cos; i++)
    {
        S.new_coroutine(readwrite_func, readwrite_arg);
    }
    S.run();
    S.close();
    delete readwrite_arg;

    // }

    return 0;
    
}
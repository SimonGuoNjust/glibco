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

static int iSuccCnt = 0;
static int iFailCnt = 0;
static int iTime = 0;

void AddSuccCnt()
{
	int now = time(NULL);
	if (now >iTime)
	{
		printf("time %d Succ Cnt %d Fail Cnt %d\n", iTime, iSuccCnt, iFailCnt);
		iTime = now;
		iSuccCnt = 0;
		iFailCnt = 0;
	}
	else
	{
		iSuccCnt++;
	}
}
void AddFailCnt()
{
	int now = time(NULL);
	if (now >iTime)
	{
		printf("time %d Succ Cnt %d Fail Cnt %d\n", iTime, iSuccCnt, iFailCnt);
		iTime = now;
		iSuccCnt = 0;
		iFailCnt = 0;
	}
	else
	{
		iFailCnt++;
	}
}

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

struct stEndPoint
{
	char *ip;
	unsigned short int port;
};

static void readwrite_func(void* arg)
{
    stEndPoint *endpoint = reinterpret_cast<stEndPoint*>(arg);
	char str[8]="sarlmol";
	char buf[1024];
	int fd = -1;
	int ret = 0;
	for(;;)
	{
		if ( fd < 0 )
		{
			fd = socket(PF_INET, SOCK_STREAM, 0);
			struct sockaddr_in addr;
			SetAddr(endpoint->ip, endpoint->port, addr);
			ret = connect(fd,(struct sockaddr*)&addr,sizeof(addr));	
			if ( errno == EALREADY || errno == EINPROGRESS )
			{       
				register_timeout<EpollScheduler>(200);
				//check connect
				int error = 0;
				uint32_t socklen = sizeof(error);
				errno = 0;
				ret = getsockopt(fd, SOL_SOCKET, SO_ERROR,(void *)&error,  &socklen);
				if ( ret == -1 ) 
				{       
					//printf("getsockopt ERROR ret %d %d:%s\n", ret, errno, strerror(errno));
					close(fd);
					fd = -1;
					AddFailCnt();
					continue;
				}       
				if ( error ) 
				{       
					errno = error;
					printf("connect ERROR ret %d %d:%s\n", error, errno, strerror(errno));
					close(fd);
					fd = -1;
					AddFailCnt();
					continue;
				}       
			} 
		}

		
		ret = write( fd,str, 8);

		if ( ret > 0 )
		{
			ret = read( fd, buf, sizeof(buf) );
			if ( ret <= 0 )
			{
				printf("co %p read ret %d errno %d (%s)\n",
						EpollScheduler::open().get_this_thread_copool()->running, ret,errno,strerror(errno));
				// while (ret <= 0 && errno == EAGAIN)
                // {
                //     ret = read( fd, buf, sizeof(buf) );
                // }
                // printf("echo %s fd %d\n", buf,fd);
                close(fd);
				fd = -1;
				AddFailCnt();
			}
			else
			{
				printf("echo %s fd %d\n", buf,fd);
				AddSuccCnt();
			}
		}
		else
		{
			printf("co %p write ret %d errno %d (%s)\n",
					EpollScheduler::open().get_this_thread_copool()->running, ret,errno,strerror(errno));
			close(fd);
			fd = -1;
			AddFailCnt();
		}
	}
};

int main(int argc,char *argv[])
{
    
    // first step ->create socket for server
    if (argc <= 3) return -1; 
    const char *ip = argv[1];
	int port = atoi( argv[2] );
    int num_co = atoi( argv[3] );
    stEndPoint endpoint;
	endpoint.ip = const_cast<char*>(ip);
	endpoint.port = port;
    printf("%d\n", num_co);
    getchar();
	// struct sigaction sa;
	// sa.sa_handler = SIG_IGN;
	// sigaction( SIGPIPE, &sa, NULL );

    EpollScheduler& S = EpollScheduler::open();
    S.start();
    for (int i = 0; i < num_co; i++)
    {
        S.new_coroutine(readwrite_func, &endpoint);
    }

    S.run();
    S.close();
    

    // }

    return 0;
    
}
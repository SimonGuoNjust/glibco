#ifdef __linux__


#include <sys/socket.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <sys/epoll.h>

#include <dlfcn.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <thread>
#include <array>
#include <netinet/in.h>

#include <stdio.h>
#include <stdarg.h>
#include "glibco_all.hpp"

template <typename CoroutineType,
typename MemoryManager>
class EpollManager;

typedef ThreadPoolCoScheduler<EpollCoScheduler<Coroutine, StackPool>> EpollScheduler;

enum {__EPOLL_SIZE = 10240};

typedef int (*socket_pfn_t)(int domain, int type, int protocol);
typedef int (*connect_pfn_t)(int socket, const struct sockaddr *address, socklen_t address_len);
typedef int (*close_pfn_t)(int fd);

typedef ssize_t (*read_pfn_t)(int fildes, void *buf, size_t nbyte);
typedef ssize_t (*write_pfn_t)(int fildes, const void *buf, size_t nbyte);

typedef ssize_t (*sendto_pfn_t)(int socket, const void *message, size_t length,
	                 int flags, const struct sockaddr *dest_addr,
					               socklen_t dest_len);

typedef ssize_t (*recvfrom_pfn_t)(int socket, void *buffer, size_t length,
	                 int flags, struct sockaddr *address,
					               socklen_t *address_len);

typedef size_t (*send_pfn_t)(int socket, const void *buffer, size_t length, int flags);
typedef ssize_t (*recv_pfn_t)(int socket, void *buffer, size_t length, int flags);

typedef int (*poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);
typedef int (*setsockopt_pfn_t)(int socket, int level, int option_name,
			                 const void *option_value, socklen_t option_len);

typedef int (*fcntl_pfn_t)(int fildes, int cmd, ...);

typedef int (*epoll_wait_pfn_t) (int __epfd, struct epoll_event *__events,
		       int __maxevents, int __timeout);
typedef int (*epoll_ctl_pfn_t) (int __epfd, int __op, int __fd,
		      struct epoll_event *__event);
typedef int (*epoll_create_pfn_t) (int size);

static socket_pfn_t g_sys_socket_func 	= (socket_pfn_t)dlsym(RTLD_NEXT,"socket");
static connect_pfn_t g_sys_connect_func = (connect_pfn_t)dlsym(RTLD_NEXT,"connect");
static close_pfn_t g_sys_close_func 	= (close_pfn_t)dlsym(RTLD_NEXT,"close");

static read_pfn_t g_sys_read_func 		= (read_pfn_t)dlsym(RTLD_NEXT,"read");
static write_pfn_t g_sys_write_func 	= (write_pfn_t)dlsym(RTLD_NEXT,"write");

static sendto_pfn_t g_sys_sendto_func 	= (sendto_pfn_t)dlsym(RTLD_NEXT,"sendto");
static recvfrom_pfn_t g_sys_recvfrom_func = (recvfrom_pfn_t)dlsym(RTLD_NEXT,"recvfrom");

static send_pfn_t g_sys_send_func 		= (send_pfn_t)dlsym(RTLD_NEXT,"send");
static recv_pfn_t g_sys_recv_func 		= (recv_pfn_t)dlsym(RTLD_NEXT,"recv");

static epoll_create_pfn_t g_sys_epoll_create_func 	= (epoll_create_pfn_t )dlsym(RTLD_NEXT,"epoll_create");
static epoll_ctl_pfn_t g_sys_epoll_create_func 	= (epoll_ctl_pfn_t )dlsym(RTLD_NEXT,"epoll_ctl");
static epoll_wait_pfn_t g_sys_epoll_create_func 	= (epoll_wait_pfn_t )dlsym(RTLD_NEXT,"epoll_wait");

static setsockopt_pfn_t g_sys_setsockopt_func 
										= (setsockopt_pfn_t)dlsym(RTLD_NEXT,"setsockopt");
static fcntl_pfn_t g_sys_fcntl_func 	= (fcntl_pfn_t)dlsym(RTLD_NEXT,"fcntl");

#define HOOK_SYS_FUNC(name) if( !g_sys_##name##_func ) { g_sys_##name##_func = (name##_pfn_t)dlsym(RTLD_NEXT,#name); }

template <typename CoroutineType,
typename MemoryManager>
class EpollCoScheduler : public CoScheduler
{
public:
	EpollCoScheduler(): CoScheduler(), epoll_fd(epoll_create(__EPOLL_SIZE)) {
		epoll_events_ = (struct epoll_event*)calloc( 1, __EPOLL_SIZE * sizeof( struct epoll_event ) );
	}

	void run()
	{
		for(;;)
        {
			process_epoll();
            process_timeout();
            run_one();
            if (nco <= 0) break;
        }
	}

	int run_epoll_ctl(int op, int fd, struct epoll_event * ev)
	{
		ev->data.ptr = reinterpret_cast<void*>(running);
		return epoll_ctl(epoll_fd, op, fd, ev);
	}

private:

	int process_epoll()
	{
		int ret = epoll_wait(epoll_id, epoll_events_, __EPOLL_SIZE, 0);
	}

	int epoll_fd;
	struct epoll_event* epoll_events_;

};

struct EpollTask : public TimerTask
{
	int fd;
	struct epoll_event events;

	void* run_co;

};

struct SocketInfo
{
    int user_flag;
    struct sockaddr_in dest;
    int domain;

    struct timeval read_timeout;
    struct timeval write_timeout;
};

class SocketInfoManager
{
public:
    inline SocketInfo* get(int fd)
    {
        if (fd > -1 && fd < socket_fd_infos.size() - 1)
        {
            return socket_fd_infos[fd];
        }
        return nullptr;
    }

    inline SocketInfo* set(int fd)
    {
        if (fd > -1 && fd < socket_fd_infos.size() - 1)
        {
            SocketInfo* new_info = new SocketInfo();
            new_info -> read_timeout.tv_sec = 1;
            new_info -> write_timeout.tv_sec = 1;
            socket_fd_infos[fd] = new_info;
            return new_info;
        }
        return nullptr;
    }

    inline void del(int fd)
    {
        if (fd > -1 && fd < socket_fd_infos.size() - 1)
        {
            SocketInfo* info = socket_fd_infos[fd];
            if ( info )
            {
                socket_fd_infos[fd] = nullptr;
                delete info;
            }
        }
    }

    static SocketInfoManager& open()
    {
        static SocketInfoManager info_manager;
        return info_manager;
    }

private:
    SocketInfoManager() {};
    std::array<SocketInfo*, 102400> socket_fd_infos = {0};
};

int socket(int domain, int type, int protocol)
{
	HOOK_SYS_FUNC( socket );

	int fd = g_sys_socket_func(domain,type,protocol);
	if( fd < 0 )
	{
		return fd;
	}

	SocketInfo* info = SocketInfoManager::open().set( fd );
	info->domain = domain;
	
	// 在fcntl函数中，会将fd变成非阻塞的
	// flag |= O_NONBLOCK;
	fcntl( fd, F_SETFL, g_sys_fcntl_func(fd, F_GETFL,0 ) );

	return fd;
}


int fcntl(int fildes, int cmd, ...)
{
	HOOK_SYS_FUNC( fcntl );

	if( fildes < 0 )
	{
		return __LINE__;
	}

	va_list arg_list;
	va_start( arg_list,cmd );

	int ret = -1;
	SocketInfo* lp = SocketInfoManager::open().get( fildes );
	switch( cmd )
	{
		case F_DUPFD:
		{
			int param = va_arg(arg_list,int);
			ret = g_sys_fcntl_func( fildes,cmd,param );
			break;
		}
		case F_GETFD:
		{
			ret = g_sys_fcntl_func( fildes,cmd );
			break;
		}
		case F_SETFD:
		{
			int param = va_arg(arg_list,int);
			ret = g_sys_fcntl_func( fildes,cmd,param );
			break;
		}
		case F_GETFL:
		{
			ret = g_sys_fcntl_func( fildes,cmd );
			break;
		}
		case F_SETFL:
		{
			int param = va_arg(arg_list,int);
			int flag = param;

			// 如果开启了系统hook，并且lp不等于null
			// lp不等于null说明这个fd是被系统hook拦截的
			// 则将该fd置为O_NONBLOCK
			if( lp )
			{
				flag |= O_NONBLOCK;
			}
			ret = g_sys_fcntl_func( fildes,cmd,flag );

			if( 0 == ret && lp )
			{
				// 注意这里的user_flag并没有包含libco设置的O_NONBLOCK
				// 仅仅是用户进行的设置
				lp->user_flag = param;
			}
			break;
		}
		case F_GETOWN:
		{
			ret = g_sys_fcntl_func( fildes,cmd );
			break;
		}
		case F_SETOWN:
		{
			int param = va_arg(arg_list,int);
			ret = g_sys_fcntl_func( fildes,cmd,param );
			break;
		}
		case F_GETLK:
		{
			struct flock *param = va_arg(arg_list,struct flock *);
			ret = g_sys_fcntl_func( fildes,cmd,param );
			break;
		}
		case F_SETLK:
		{
			struct flock *param = va_arg(arg_list,struct flock *);
			ret = g_sys_fcntl_func( fildes,cmd,param );
			break;
		}
		case F_SETLKW:
		{
			struct flock *param = va_arg(arg_list,struct flock *);
			ret = g_sys_fcntl_func( fildes,cmd,param );
			break;
		}
	}

	va_end( arg_list );

	return ret;
}


int connect(int fd, const struct sockaddr *address, socklen_t address_len)
{
	HOOK_SYS_FUNC( connect );

	int ret = g_sys_connect_func( fd,address,address_len );

	SocketInfo *lp = SocketInfoManager::open().get( fd );
	if( !lp ) return ret;

	if( sizeof(lp->dest) >= address_len )
	{
		 memcpy( &(lp->dest),address,(int)address_len );
	}
	if( O_NONBLOCK & lp->user_flag ) 
	{
		return ret;
	}
	
	if (!(ret < 0 && errno == EINPROGRESS))
	{
		return ret;
	}

	int pollret = 0;
	struct pollfd pf = { 0 };

	// 75s是内核默认的超时时间
	for(int i=0;i<3;i++) //25s * 3 = 75s
	{
		memset( &pf,0,sizeof(pf) );
		pf.fd = fd;
		pf.events = ( POLLOUT | POLLERR | POLLHUP );

		pollret = poll( &pf,1,25000 );

		if( 1 == pollret  )
		{
			break;
		}
	}
	if( pf.revents & POLLOUT ) //connect succ
	{
		errno = 0;
		return 0;
	}

	//3.set errno
	int err = 0;
	socklen_t errlen = sizeof(err);
	getsockopt( fd,SOL_SOCKET,SO_ERROR,&err,&errlen);
	if( err ) 
	{
		errno = err;
	}
	else
	{
		errno = ETIMEDOUT;
	} 
	return ret;
}

int close(int fd)
{
	HOOK_SYS_FUNC( close );

	SocketInfoManager::open().del( fd );
	int ret = g_sys_close_func(fd);

	return ret;
}

ssize_t read( int fd, void *buf, size_t nbyte )
{
	HOOK_SYS_FUNC( read );

    SocketInfo *lp = SocketInfoManager::open().get( fd );

	if( !lp || ( O_NONBLOCK & lp->user_flag ) ) 
	{
		ssize_t ret = g_sys_read_func( fd,buf,nbyte );
		return ret;
	}

	int timeout = ( lp->read_timeout.tv_sec * 1000 ) 
				+ ( lp->read_timeout.tv_usec / 1000 );

    
	EpollTask* arg = new EpollTask;
	arg->events.events = EPOLLIN | EPOLLERR | EPOLLHUP;

	auto co_sch = EpollScheduler::open().get_this_thread_copool();
	int ret = EpollScheduler::open().get_this_thread_copool() -> run_epoll_ctl(EPOLL_CTL_ADD, fd, &(arg->events));

	ssize_t readret = g_sys_read_func( fd,(char*)buf ,nbyte );

	if( readret < 0 )
	{
		printf("CO_ERR: read fd %d ret %ld errno %d poll ret %d timeout %d",
					fd,readret,errno, ret,timeout);
	}

	return readret;

}

ssize_t write( int fd, const void *buf, size_t nbyte )
{
	HOOK_SYS_FUNC( write );
	
    SocketInfo *lp = SocketInfoManager::open().get( fd );

	if( !lp || ( O_NONBLOCK & lp->user_flag ) )
	{
		ssize_t ret = g_sys_write_func( fd,buf,nbyte );
		return ret;
	}
	size_t wrotelen = 0;
	int timeout = ( lp->write_timeout.tv_sec * 1000 ) 
				+ ( lp->write_timeout.tv_usec / 1000 );

	ssize_t writeret = g_sys_write_func( fd,(const char*)buf + wrotelen,nbyte - wrotelen );

	if (writeret == 0)
	{
		return writeret;
	}

	if( writeret > 0 )
	{
		wrotelen += writeret;	
	}

	while( wrotelen < nbyte )
	{
		struct pollfd pf = { 0 };
		pf.fd = fd;
		pf.events = ( POLLOUT | POLLERR | POLLHUP );

		//监听可读事件
		poll( &pf,1,timeout );

		writeret = g_sys_write_func( fd,(const char*)buf + wrotelen,nbyte - wrotelen );
		
		if( writeret <= 0 )
		{
			break;
		}
		wrotelen += writeret ;
	}
	if (writeret <= 0 && wrotelen == 0)
	{
		return writeret;
	}
	return wrotelen;
}

int schedule_task(struct pollfd fds[], nfds_t nfds, int timeout, poll_pfn_t pollfunc)
{
    
}


#endif
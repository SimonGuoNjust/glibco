#include "multithread_scheduler.hpp"

#include <sys/epoll.h>


enum {__EPOLL_SIZE = 10240};

template <typename CoroutineType,
typename MemoryManager>
class EpollCoScheduler : public CoScheduler<CoroutineType, MemoryManager>
{
public:
	EpollCoScheduler(): epoll_fd(epoll_create(__EPOLL_SIZE)) {
		epoll_events_ = (struct epoll_event*)calloc( 1, __EPOLL_SIZE * sizeof( struct epoll_event ) );
	}

    void move_coroutine_timeout(int timeout_, TimerTask* tm)
    {
        tm -> pArg = this->running;
        unsigned long long now = GetTickMS();
        tm->ullExpireTime = now + timeout_;
        int ret = this -> pTimeout -> addTask(now, tm);
        if (ret < 0)
        {
            std::cout << "add task error" << std::endl;
            delete tm;
            return;
        }
        this->running->self -> yield(WAITING);
    }
    
	int run_epoll_ctl(TimerTask* arg, int op, int fd, struct epoll_event * ev, int timeout_)
	{
		int ret = epoll_ctl(epoll_fd, op, fd, ev);

        if (ret <0 && errno == EPERM)
        {
            printf("epoll register error %d: %s \n", errno, strerror(errno));
            delete arg;
            return -1;
        }
        else
        {
            this->move_coroutine_timeout(timeout_, arg);
            RemoveFromLink<TimerTask, TimerTaskLink>(arg);
        }
        ev->data.ptr=nullptr;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, ev);
        delete arg;

		return ret;
	}

    static void global_run(void* objectPtr)
    {
        EpollCoScheduler* this_ = reinterpret_cast<EpollCoScheduler*>(objectPtr);
        this_->run();
    }

    void run()
    {
        for(;;)
        {
            this->process_timeout();
            this->run_one();
            if (this->nco <= 0) break;
        }
    }

	void process_timeout()
    {
        TimerTaskLink timeout_list;
        TimerTaskLink* ptimeout_list = &timeout_list;
		TimerTaskLink epoll_awake_list;
		TimerTaskLink* pepoll_awake_list = &epoll_awake_list;
        memset(pepoll_awake_list, 0, sizeof(epoll_awake_list));
        
		process_epoll(pepoll_awake_list);

        unsigned long long now = GetTickMS();
        memset(ptimeout_list, 0, sizeof(TimerTaskLink));
        this->pTimeout->getAllTimeout(now, ptimeout_list);

        TimerTask* p = ptimeout_list->head;
        while(p)
        {
            p -> bTimeout = true;
            p = p->pNext;
        }

		Join<TimerTask, TimerTaskLink>(pepoll_awake_list, ptimeout_list);

        p = pepoll_awake_list -> head;
        while(p)
        {
            PopHead<TimerTask, TimerTaskLink>(pepoll_awake_list);
            if (p->pfnProcess)
            {
                this->pTimeout->do_timeout(p);
            }
            if (p -> needClean)
            {
                delete p;
            }
            p = pepoll_awake_list->head;
        }
    }

	void process_epoll(TimerTaskLink* list)
	{
		int ret = epoll_wait(epoll_fd, epoll_events_, __EPOLL_SIZE, 1);
		for (int i = 0; i < ret; i++)
		{
			TimerTask* p = reinterpret_cast<TimerTask*>(epoll_events_[i].data.ptr);
			AddTail(list, p);
		}
	}

private:
	int epoll_fd;
	struct epoll_event* epoll_events_;

};
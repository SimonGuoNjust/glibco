#ifndef __TIMEWHEEL__
#define __TIMEWHEEL__
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <iostream>
#include "task_queue.hpp"
#ifdef __linux__
#include <sys/time.h>
#else
#include <windows.h>
#endif
struct TimerTask;
typedef TaskLink<TimerTask> TimerTaskLink;
class Coroutine;
typedef void (*OnPreparePfn_t)( TimerTask *,struct epoll_event &ev, TimerTaskLink *active );
typedef void (*OnProcessPfn_t)( Coroutine *);
unsigned long long GetTickMS();

struct TimerTask
{
	enum
	{
		eMaxTimeout = 40 * 1000 //40s
	};

	TimerTask *pPrev;	// 前一个元素
	TimerTask *pNext; // 后一个元素

	TimerTaskLink *pLink; // 该链表项所属的链表

	unsigned long long ullExpireTime;

	OnPreparePfn_t pfnPrepare;  // 预处理函数，在eventloop中会被调用
	OnProcessPfn_t pfnProcess;  // 处理函数 在eventloop中会被调用

	void *pArg; // self routine pArg 是pfnPrepare和pfnProcess的参数

	bool bTimeout; // 是否已经超时
	int co_id;
};



class TimeWheel
{
public:
	TimeWheel(int cap);
    ~TimeWheel();
    int addTask(unsigned long long now, TimerTask* task);
	void getAllTimeout(unsigned long long now, TimerTaskLink* result);

private:
    TimerTaskLink* pItems;
	int iItemSize;   // 默认为60*1000

	unsigned long long start_; //目前的超时管理器最早的时间
	long long startIdx_;  //目前最早的时间所对应的pItems上的索引
};

#ifndef __linux__
int gettimeofday(struct timeval *tp, void *tzp)
{
  time_t clock;
  struct tm tm;
  SYSTEMTIME wtm;
  GetLocalTime(&wtm);
  tm.tm_year   = wtm.wYear - 1900;
  tm.tm_mon   = wtm.wMonth - 1;
  tm.tm_mday   = wtm.wDay;
  tm.tm_hour   = wtm.wHour;
  tm.tm_min   = wtm.wMinute;
  tm.tm_sec   = wtm.wSecond;
  tm. tm_isdst  = -1;
  clock = mktime(&tm);
  tp->tv_sec = clock;
  tp->tv_usec = wtm.wMilliseconds * 1000;
  return (0);
}
#endif

unsigned long long GetTickMS()
{
    struct timeval now = { 0 };
	gettimeofday( &now,NULL );
	unsigned long long u = now.tv_sec;
	u *= 1000;
	u += now.tv_usec / 1000;
	return u;
}

TimeWheel::TimeWheel(int cap)
{	
    iItemSize = cap;
    pItems = (TimerTaskLink*)calloc( 1, sizeof(TimerTaskLink) * iItemSize );

    start_ = GetTickMS();
    startIdx_ = 0;
}

TimeWheel::~TimeWheel()
{
    free(pItems);
}

int TimeWheel::addTask(unsigned long long now, TimerTask* task)
{
        // 当前时间管理器的最早超时时间
    if(start_ == 0 )
    { 
        // 设置时间轮的最早时间是当前时间
        start_ = now;
        // 设置最早时间对应的index 为 0
        startIdx_ = 0;
    }

    if( now < start_ )
    {
        // co_log_err("CO_ERR: AddTimeout line %d allNow %llu apTimeout->ullStart %llu",
        //             __LINE__,allNow,apTimeout->ullStart);

        return -1;
    }

    if( task->ullExpireTime < now )
    {
        // co_log_err("CO_ERR: AddTimeout line %d apItem->ullExpireTime %llu allNow %llu apTimeout->ullStart %llu",
        //             __LINE__,apItem->ullExpireTime,allNow,apTimeout->ullStart);

        return -1;
    }

    // 计算当前事件的超时时间和超时管理器的最早时间的差距
    int diff = task->ullExpireTime - start_;

    if( diff >= iItemSize )
    {
        // co_log_err("CO_ERR: AddTimeout line %d diff %d",
        //             __LINE__,diff);

        return -1;
    }
    /*
    计算出该事件的超时事件在超时管理器所在的槽的位置
    apTimeout->pItems + ( apTimeout->llStartIdx + diff ) % apTimeout->iItemSize , apItem );
    
    然后在该位置的槽对应的超时链表的尾部添加一个事件
    */
    AddTail( pItems + ( startIdx_ + diff ) % iItemSize , task );

    return 0;
}

void TimeWheel::getAllTimeout(unsigned long long now, TimerTaskLink* result)
{
    if (start_ == 0)
    {
        start_ = now;
        startIdx_ = 0;
    }

    if (now < start_)
    {
        return;
    }

    int cnt = now - start_ + 1;
    if (cnt > iItemSize)
    {
        cnt = iItemSize;
    }
    if (cnt < 0)
    {
        return;
    }
    for (int i = 0; i < cnt; i++)
    {
        int idx = ( startIdx_ + i ) % iItemSize;
        Join<TimerTask, TimerTaskLink>(result, pItems + idx); 
    }
    start_ = now;
    startIdx_ += cnt - 1;
}

#endif
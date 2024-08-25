#ifndef __TIMEWHEEL__
#define __TIMEWHEEL__
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <iostream>
#ifdef __linux__
#include <sys/time.h>
#else
#include <windows.h>
#endif
struct TimerTask;
struct TimerTaskLink;
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

struct TimerTaskLink
{
    TimerTask* head;
    TimerTask* tail;
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

template <class T,class TLink>
void RemoveFromLink(T *ap)
{
	// pLink代表ap所属的链表
	TLink *lst = ap->pLink;

	if(!lst) return ;
	assert( lst->head && lst->tail );

	if( ap == lst->head )
	{
		lst->head = ap->pNext;
		if(lst->head)
		{
			lst->head->pPrev = NULL;
		}
	}
	else
	{
		if(ap->pPrev)
		{
			ap->pPrev->pNext = ap->pNext;
		}
	}

	if( ap == lst->tail )
	{
		lst->tail = ap->pPrev;
		if(lst->tail)
		{
			lst->tail->pNext = NULL;
		}
	}
	else
	{
		ap->pNext->pPrev = ap->pPrev;
	}

	ap->pPrev = ap->pNext = NULL;
	ap->pLink = NULL;
}

template <class TNode,class TLink>
void inline AddTail(TLink*apLink,TNode *ap)
{
	if( ap->pLink )
	{
		return ;
	}
	if(apLink->tail)
	{
		apLink->tail->pNext = (TNode*)ap;
		ap->pNext = NULL;
		ap->pPrev = apLink->tail;
		apLink->tail = ap;
	}
	else
	{
		apLink->head = apLink->tail = ap;
		ap->pNext = ap->pPrev = NULL;
	}
	ap->pLink = apLink;
}

template <class TNode,class TLink>
void inline PopHead( TLink*apLink )
{
	if( !apLink->head ) 
	{
		return ;
	}
	TNode *lp = apLink->head;
	if( apLink->head == apLink->tail )
	{
		apLink->head = apLink->tail = NULL;
	}
	else
	{
		apLink->head = apLink->head->pNext;
	}

	lp->pPrev = lp->pNext = NULL;
	lp->pLink = NULL;

	if( apLink->head )
	{
		apLink->head->pPrev = NULL;
	}
}

template <class TNode,class TLink>
void inline Join( TLink*apLink,TLink *apOther )
{
	//printf("apOther %p\n",apOther);
	if( !apOther->head )
	{
		return ;
	}
	TNode *lp = apOther->head;
	while( lp )
	{
		lp->pLink = apLink;
		lp = lp->pNext;
	}
	lp = apOther->head;
	if(apLink->tail)
	{
		apLink->tail->pNext = (TNode*)lp;
		lp->pPrev = apLink->tail;
		apLink->tail = apOther->tail;
	}
	else
	{
		apLink->head = apOther->head;
		apLink->tail = apOther->tail;
	}

	apOther->head = apOther->tail = NULL;
}
#endif
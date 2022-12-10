/*************************************************************************
	> File Name: Fiber.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月30日 星期日 17时06分10秒
 ************************************************************************/

#ifndef _FIBER_H
#define _FIBER_H

#include <memory>
#include <functional>

#define FIBER_UCONTEXT 				1
#define FIBER_LIBCO					2

#ifndef FIBER_TYPE
#define FIBER_TYPE	FIBER_LIBCO
#endif

#if FIBER_TYPE == FIBER_UCONTEXT
#include <ucontext.h>
#elif FIBER_TYPE == FIBER_LIBCO
#include "plugins/libco/coctx.h"
#endif

#include "Thread.h"


namespace Routn
{
	class Schedular;

	class Fiber : public std::enable_shared_from_this<Fiber>
	{
		friend class Schedular;
	public:
		using ptr = std::shared_ptr<Fiber>;
		enum STATUS
		{
			INIT,
			HOLD,
			EXEC,
			TERM,
			READY,
			EXCEPT
		};

	public:
		Fiber();
		Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
		~Fiber();

		//重置协程函数，并重置状态（INIT，TERM）
		void reset(std::function<void()> cb);
		//切换到当前协程执行
		void swapIn();
		//切换到后台执行
		void swapOut();
		//当前协程切换至目标执行协程
		void call();
		void back();

		uint64_t getId() const { return _fid; }
		STATUS getStatus() const { return _fstatus; }
		void setStatus(STATUS val) { _fstatus = val; }

	public:
		//设置当前协程
		static void SetThis(Fiber* f);
		//返回当前协程
		static Fiber::ptr GetThis();
		//协程切换到后台，并且设置为Ready状态
		static void YieldToReady();
		//协程切换到后台，并且设置为Hold状态
		static void YieldToHold();
		//获取总的协程数
		static uint64_t TotalFibers();
#if FIBER_TYPE == FIBER_UCONTEXT	
		static void MainFunc();
		static void MainFuncCaller();
#elif FIBER_TYPE == FIBER_LIBCO
		static void* MainFunc(void *, void *);
		static void* MainFuncCaller(void *, void *);
#endif
		
		static uint64_t GetFiberId();
	
		
	private:
		uint64_t _fid = 0;
		uint32_t _fstacksize = 0;
		STATUS _fstatus = INIT;
#if FIBER_TYPE == FIBER_UCONTEXT
		ucontext_t _fctx;
#elif FIBER_TYPE == FIBER_LIBCO
		coctx_t _fctx;
#endif
		void * _fstack = nullptr;

		std::function<void()> _fcallback;
	};


	class MallocStackAllocator
	{
	public:
		static void* Alloc(size_t size)
		{
			return malloc(size);
		}
		static void Dealloc(void *vp, size_t size)
		{
			return free(vp);
		}
	};
	
	
	Fiber* NewFiber();
	Fiber* NewFiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
	void FreeFiber(Fiber* ptr);


}

#endif

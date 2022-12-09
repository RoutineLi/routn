/*************************************************************************
	> File Name: Fiber.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月30日 星期日 17时06分45秒
 ************************************************************************/

#include "Fiber.h"
#include "Config.h"
#include "Log.h"
#include "Macro.h"
#include "Scheduler.h"
#include <atomic>

namespace Routn
{
	static std::atomic<uint64_t> s_fiber_id{0};
	static std::atomic<uint64_t> s_fiber_count{0};

	static thread_local Fiber *s_t_fiber = nullptr;
	static thread_local Fiber::ptr t_threadFiber = nullptr;

	static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
		Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");
		

	using StackAllocator = MallocStackAllocator;

	Fiber::Fiber()
	{
		_fstatus = EXEC;
		SetThis(this);
#if FIBER_TYPE == FIBER_UCONTEXT
		if (getcontext(&_fctx))
		{
			ROUTN_ASSERT_ARG(false, "getcontext");
		}
#elif FIBER_TYPE == FIBER_LIBCO
		coctx_init(&_fctx);
		++s_fiber_count;
#endif
		ROUTN_LOG_DEBUG(g_logger) << "Fiber::Fiber()";
	}

	Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
		: _fid(++s_fiber_id), _fcallback(cb)
	{
		++s_fiber_count;
		_fstacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
		//_fstacksize = stacksize;

		_fstack = StackAllocator::Alloc(_fstacksize);
#if FIBER_TYPE == FIBER_UCONTEXT
		if (getcontext(&_fctx))
		{
			ROUTN_ASSERT_ARG(false, "getcontext");
		}
		_fctx.uc_link = nullptr;
		_fctx.uc_stack.ss_sp = _fstack;
		_fctx.uc_stack.ss_size = _fstacksize;
		///std::cout << L_GREEN << "ucontext" << _NONE << std::endl;
		if (!use_caller)
		{
			makecontext(&_fctx, &Fiber::MainFunc, 0);
		}
		else
		{
			makecontext(&_fctx, &Fiber::MainFuncCaller, 0);
		}
#elif FIBER_TYPE == FIBER_LIBCO
		_fctx.ss_size = _fstacksize;
		_fctx.ss_sp = (char *)_fstack;
		///std::cout << L_GREEN << "libco" << _NONE << std::endl;
		if (!use_caller)
		{
			coctx_make(&_fctx, &Fiber::MainFunc, 0, 0);
		}
		else
		{
			coctx_make(&_fctx, &Fiber::MainFuncCaller, 0, 0);
		}
#endif
		ROUTN_LOG_DEBUG(g_logger) << "Fiber::Fiber(std::function<void()> cb, size_t stacksize) id = " << _fid;
	}

	Fiber::~Fiber()
	{
		--s_fiber_count;
		if (_fstack)
		{
			ROUTN_ASSERT(_fstatus == TERM || _fstatus == EXCEPT || _fstatus == INIT);

			StackAllocator::Dealloc(_fstack, _fstacksize);
		}
		else
		{
			ROUTN_ASSERT(!_fcallback);
			ROUTN_ASSERT(_fstatus == EXEC);

			Fiber *cur = s_t_fiber;
			if (cur == this)
			{
				SetThis(nullptr);
			}
		}
		ROUTN_LOG_DEBUG(g_logger) << "Fiber::~Fiber() id = " << _fid
								  << " total fibers = [" << s_fiber_count << "]";
	}

	//重置协程函数，并重置状态（INIT，TERM）
	void Fiber::reset(std::function<void()> cb)
	{
		ROUTN_ASSERT(_fstack);
		ROUTN_ASSERT(_fstatus == TERM || _fstatus == EXCEPT || _fstatus == INIT);
		_fcallback = cb;

#if FIBER_TYPE == FIBER_UCONTEXT
		if (getcontext(&_fctx))
		{
			ROUTN_ASSERT_ARG(false, "getcontext");
		}

		_fctx.uc_link = nullptr;
		_fctx.uc_stack.ss_sp = _fstack;
		_fctx.uc_stack.ss_size = _fstacksize;

		makecontext(&_fctx, &Fiber::MainFunc, 0);
#elif FIBER_TYPE == FIBER_LIBCO
		coctx_make(&_fctx, &Fiber::MainFunc, 0, 0);
#endif
		_fstatus = INIT;
	}

	void Fiber::call()
	{
		SetThis(this);
		_fstatus = EXEC;
#if FIBER_TYPE == FIBER_UCONTEXT
		if (swapcontext(&t_threadFiber->_fctx, &_fctx))
		{
			ROUTN_ASSERT_ARG(false, "swapcontext");
		}
#elif FIBER_TYPE == FIBER_LIBCO
		coctx_swap(&t_threadFiber->_fctx, &_fctx);
#endif
	}

	void Fiber::back()
	{
		SetThis(t_threadFiber.get());
#if FIBER_TYPE == FIBER_UCONTEXT
		if (swapcontext(&_fctx, &t_threadFiber->_fctx))
		{
			ROUTN_ASSERT_ARG(false, "swapcontext");
		}
#elif FIBER_TYPE == FIBER_LIBCO
		coctx_swap(&_fctx, &t_threadFiber->_fctx);
#endif
	}

	//切换到当前协程执行-------------->coroutine-resume
	void Fiber::swapIn()
	{
		SetThis(this);
		ROUTN_ASSERT(_fstatus != EXEC);
		_fstatus = EXEC;
#if FIBER_TYPE == FIBER_UCONTEXT
		if (swapcontext(&Schedular::GetMainFiber()->_fctx, &_fctx))
		{
			ROUTN_ASSERT_ARG(false, "swapcontext");
		}
#elif FIBER_TYPE == FIBER_LIBCO
		coctx_swap(&Schedular::GetMainFiber()->_fctx, &_fctx);
#endif
	}

	//切换到后台执行
	void Fiber::swapOut()
	{
		SetThis(Schedular::GetMainFiber());
#if FIBER_TYPE == FIBER_UCONTEXT
		if (swapcontext(&_fctx, &Schedular::GetMainFiber()->_fctx))
		{
			ROUTN_ASSERT_ARG(false, "swapcontext");
		}
#elif FIBER_TYPE == FIBER_LIBCO
		coctx_swap(&_fctx, &Schedular::GetMainFiber()->_fctx);
#endif
	}

	//返回当前协程
	Fiber::ptr Fiber::GetThis()
	{
		if (s_t_fiber)
		{
			return s_t_fiber->shared_from_this();
		}
		Fiber::ptr main_fiber(new Fiber);
		ROUTN_ASSERT(s_t_fiber == main_fiber.get());
		t_threadFiber = main_fiber;
		return s_t_fiber->shared_from_this();
	}

	//设置当前协程
	void Fiber::SetThis(Fiber *f)
	{
		s_t_fiber = f;
	}

	//协程切换到后台，并且设置为Ready状态
	void Fiber::YieldToReady()
	{
		Fiber::ptr cur = GetThis();
		cur->_fstatus = READY;
		cur->swapOut();
	}

	//协程切换到后台，并且设置为Hold状态
	void Fiber::YieldToHold()
	{
		Fiber::ptr cur = GetThis();
		ROUTN_ASSERT(cur->_fstatus == EXEC);
		//cur->_fstatus = HOLD;
		cur->swapOut();
	}

	//获取总的协程数
	uint64_t Fiber::TotalFibers()
	{
		return s_fiber_count;
	}

	uint64_t Fiber::GetFiberId()
	{
		if (s_t_fiber)
		{
			return s_t_fiber->getId();
		}
		return 0;
	}

	//执行函数

#if FIBER_TYPE == FIBER_UCONTEXT
	void Fiber::MainFunc()
	{
#elif FIBER_TYPE == FIBER_LIBCO
	void *Fiber::MainFunc(void *, void *)
	{
#endif
		Fiber::ptr cur = GetThis();
		ROUTN_ASSERT(cur);
		try
		{
			cur->_fcallback();
			cur->_fcallback = nullptr;
			cur->_fstatus = TERM;
		}
		catch (std::exception &e)
		{
			cur->_fstatus = EXCEPT;
			ROUTN_LOG_ERROR(g_logger) << "Fiber Except:" << e.what();
		}
		catch (...)
		{
			cur->_fstatus = EXCEPT;
			ROUTN_LOG_ERROR(g_logger) << "Fiber Except";
		}

		auto naked_ptr = cur.get();
		cur.reset();
		naked_ptr->swapOut();

		ROUTN_ASSERT_ARG(false, "never reach");
	}

	//call back 调用该执行函数
#if FIBER_TYPE == FIBER_UCONTEXT
	void Fiber::MainFuncCaller(){
#elif FIBER_TYPE == FIBER_LIBCO
	void *Fiber::MainFuncCaller(void *, void *)
	{
#endif
		Fiber::ptr cur = GetThis();
	ROUTN_ASSERT(cur);
	try
	{
		cur->_fcallback();
		cur->_fcallback = nullptr;
		cur->_fstatus = TERM;
	}
	catch (std::exception &e)
	{
		cur->_fstatus = EXCEPT;
		ROUTN_LOG_ERROR(g_logger) << "Fiber Except:" << e.what();
	}
	catch (...)
	{
		cur->_fstatus = EXCEPT;
		ROUTN_LOG_ERROR(g_logger) << "Fiber Except";
	}

	auto naked_ptr = cur.get();
	cur.reset();
	naked_ptr->back();

	ROUTN_ASSERT_ARG(false, "never reach");
}

Fiber *NewFiber()
{
	auto fiber = new Routn::Fiber();
	return fiber;
}

Fiber *NewFiber(std::function<void()> cb, size_t stacksize, bool use_caller)
{
	stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
	Fiber *p = (Fiber *)StackAllocator::Alloc(sizeof(Fiber) + stacksize);
	return new (p) Fiber(cb, stacksize, use_caller);
}

void FreeFiber(Fiber *ptr)
{
	ptr->~Fiber();
	StackAllocator::Dealloc(ptr, 0);
}
}
;

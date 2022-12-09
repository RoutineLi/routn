/*************************************************************************
	> File Name: FiberSem.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月28日 星期一 01时07分29秒
 ************************************************************************/

#ifndef _FIBERSEM_H
#define _FIBERSEM_H

#include "Scheduler.h"
#include "Noncopyable.h"
#include <cstdint>

namespace Routn{
	class FiberSemaphore : Noncopyable {
	public:
		using MutexType = SpinLock;

		FiberSemaphore(size_t initial_concurrency = 0);
		~FiberSemaphore();

		bool tryWait();
		void wait();
		void notify();
		void notifyAll();

		size_t getConcurrency() const { return m_concurrency;}
		void reset() { m_concurrency = 0;}
	private:
		MutexType m_mutex;
		std::list<std::pair<Schedular*, Fiber::ptr> > m_waiters;
		size_t m_concurrency;
	};
}

#endif

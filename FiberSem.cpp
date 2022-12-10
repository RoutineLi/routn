/*************************************************************************
	> File Name: FiberSem.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月28日 星期一 01时07分32秒
 ************************************************************************/

#include "FiberSem.h"
#include "Macro.h"

using namespace std;

namespace Routn{
	FiberSemaphore::FiberSemaphore(size_t initial_concurrency)
    	:m_concurrency(initial_concurrency) {
	}

	FiberSemaphore::~FiberSemaphore() {
		ROUTN_ASSERT(m_waiters.empty());
	}

	bool FiberSemaphore::tryWait() {
		ROUTN_ASSERT(Schedular::GetThis())
		{
			MutexType::Lock lock(m_mutex);
			if(m_concurrency > 0u) {
				--m_concurrency;
				return true;
			}
			return false;
		}
	}

	void FiberSemaphore::wait() {
		ROUTN_ASSERT(Schedular::GetThis());
		{
			MutexType::Lock lock(m_mutex);
			if(m_concurrency > 0u) {
				--m_concurrency;
				return;
			}
			m_waiters.push_back({Schedular::GetThis(), Fiber::GetThis()});
		}
		Fiber::YieldToHold();
	}

	void FiberSemaphore::notify() {
		MutexType::Lock lock(m_mutex);
		if(!m_waiters.empty()) {
			auto next = m_waiters.front();
			m_waiters.pop_front();
			next.first->schedule(next.second);
		} else {
			++m_concurrency;
		}
	}

	void FiberSemaphore::notifyAll() {
		MutexType::Lock lock(m_mutex);
		for(auto& i : m_waiters) {
			i.first->schedule(i.second);
		}
		m_waiters.clear();
	}
}
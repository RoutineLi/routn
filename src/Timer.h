/*************************************************************************
	> File Name: Timer.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月02日 星期三 11时28分57秒
 ************************************************************************/

#ifndef _TIMER_H
#define _TIMER_H

#include <memory>
#include <set>
#include <vector>
#include "Thread.h"

namespace Routn{
    
	class TimerManager;
    class Timer : public std::enable_shared_from_this<Timer>{
	friend class TimerManager;
	public:
		using ptr = std::shared_ptr<Timer>;

	private:
		Timer(uint64_t ms, std::function<void()> cb, 
			bool recurring, TimerManager* manager);
		Timer(uint64_t next);
	public:
		bool refresh();
		bool cancel();
		bool reset(uint64_t ms, bool from_now);
	private:
		bool _recurring = false;			//是否循环定时器
		uint64_t _ms = 0;					//执行周期
		uint64_t _next = 0;					//高精度时间，下一个周期需要执行的时间
		TimerManager* _manager = nullptr;
		std::function<void()> _callback;
		
	private:
		struct Comparator{
			bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
		};
	};	


	class TimerManager{
	friend class Timer;
	public:
		using RWMutexType = RW_Mutex;

		TimerManager();
		virtual ~TimerManager();
		Timer::ptr addTimer(uint64_t ms, std::function<void()> cb
						, bool recurring = false);
		Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
						, std::weak_ptr<void> weak_cond
						, bool recurring = false);
		uint64_t getNextTimer();
		bool hasTimer();
		void listExpiredCb(std::vector<std::function<void()>> &cbs);
	protected:
		virtual void onTimerInsertedAtFront() = 0;
		void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);
	private:
		bool detectClockRollover(uint64_t now_ms);
	private:
		RWMutexType _mutex;
		std::set<Timer::ptr, Timer::Comparator> _timers;
		bool _tickled = false;
		uint64_t _previousTime = 0;
	};

}

#endif

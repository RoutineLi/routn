/*************************************************************************
	> File Name: Timer.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月02日 星期三 11时29分15秒
 ************************************************************************/

#include "Timer.h"
#include "Util.h"

namespace Routn{

	Timer::Timer(uint64_t next)
		: _next(next)
	{
		
	}

	Timer::Timer(uint64_t ms, std::function<void()> cb, 
			bool recurring, TimerManager* manager)
		: _recurring(recurring)
		, _ms(ms)
		, _manager(manager)
		, _callback(cb)
	{
		_next = Routn::GetCurrentMs() + _ms;
	}

	bool Timer::refresh()
	{
		TimerManager::RWMutexType::WriteLock lock(_manager->_mutex);
		if(!_callback){
			return false;
		}
		auto it = _manager->_timers.find(shared_from_this());
		if(it == _manager->_timers.end()){
			return false;
		}
		_manager->_timers.erase(it);
		_next = Routn::GetCurrentMs() + _ms;
		_manager->_timers.insert(shared_from_this());
		return true;
	}
	bool Timer::cancel()
	{
		TimerManager::RWMutexType::WriteLock lock(_manager->_mutex);
		if(_callback){
			_callback = nullptr;
			auto it = _manager->_timers.find(shared_from_this());
			_manager->_timers.erase(it);
			return true;
		}
		return false;
	}
	bool Timer::reset(uint64_t ms, bool from_now)
	{
		if(ms == _ms && !from_now){
			return true;
		}

		TimerManager::RWMutexType::WriteLock lock(_manager->_mutex);
		if(!_callback){
			return false;
		}
		auto it = _manager->_timers.find(shared_from_this());
		if(it == _manager->_timers.end()){
			return false;
		}
		_manager->_timers.erase(it);
		uint64_t start = 0;
		if(from_now){
			start = Routn::GetCurrentMs();
		}else{
			start = _next - _ms;
		}
		_ms = ms;
		_next = start + _ms;
		_manager->addTimer(shared_from_this(), lock);
		return true;
	}

	bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const
	{
		if(!lhs && !rhs){
			return false;
		}
		if(!lhs){
			return true;
		}
		if(!rhs){
			return false;
		}
		if(lhs->_next < rhs->_next){
			return true;
		}
		if(rhs->_next < lhs->_next){
			return false;
		}
		return lhs.get() < rhs.get();
	}
		
	TimerManager::TimerManager(){
		_previousTime = Routn::GetCurrentMs();
	}
	TimerManager::~TimerManager(){

	}
	Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb
					, bool recurring)
	{	
		Timer::ptr timer(new Timer(ms, cb, recurring, this));
		RWMutexType::WriteLock lock(_mutex);	
		addTimer(timer, lock);
		return timer;
	}

	static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
	{
		std::shared_ptr<void> temp = weak_cond.lock();
		if(temp){
			cb();
		}
	}

	Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb
					, std::weak_ptr<void> weak_cond
					, bool recurring)
	{
		return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
	}	


	uint64_t TimerManager::getNextTimer(){
		RWMutexType::ReadLock lock(_mutex);
		_tickled = false;
		if(_timers.empty()){
			return ~0ull;
		}

		const Timer::ptr& next = *_timers.begin();
		uint64_t now_ms = Routn::GetCurrentMs();
		if(now_ms >= next->_next){
			return 0;
		}else{
			return next->_next - now_ms;
		}
	}


	void TimerManager::listExpiredCb(std::vector<std::function<void()> > &cbs)
	{	
		uint64_t now_ms = Routn::GetCurrentMs();
		std::vector<Timer::ptr> expired;

		{
			RWMutexType::ReadLock lock(_mutex);
			if(_timers.empty())
				return ;		
		}
		RWMutexType::WriteLock lock(_mutex);
		if(_timers.empty()){
			return ;
		}

		bool rollover = detectClockRollover(now_ms);
		if(!rollover && ((*_timers.begin())->_next > now_ms)){
			return ;
		}
		Timer::ptr now_timer(new Timer(now_ms));
		auto it = rollover ? _timers.end() : _timers.lower_bound(now_timer);
		while(it != _timers.end() && (*it)->_next == now_ms)
		{
			++it;
		}
		expired.insert(expired.begin(), _timers.begin(), it);
		_timers.erase(_timers.begin(), it);
		cbs.reserve(expired.size());

		for(auto& timer : expired)
		{
			cbs.push_back(timer->_callback);
			if(timer->_recurring){
				timer->_next = now_ms + timer->_ms;
				_timers.insert(timer);
			}else{
				timer->_callback = nullptr;
			}
		}	
	}

	void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock)
	{	
		auto it = _timers.insert(val).first;
		bool at_front = (it == _timers.begin()) && !_tickled;
		if(at_front){
			_tickled = true;
		}
		lock.unlock();

		if(at_front){
			onTimerInsertedAtFront();
		}
	}

	bool TimerManager::detectClockRollover(uint64_t now_ms)
	{
		bool rollover = false;
		if(now_ms < _previousTime
				&& now_ms < (_previousTime - 60 * 60 * 1000)){
			rollover = true;
		}
		_previousTime = now_ms;
		return rollover;
	}

	bool TimerManager::hasTimer(){
		RWMutexType::ReadLock lock(_mutex);
		return !_timers.empty();
	}
}

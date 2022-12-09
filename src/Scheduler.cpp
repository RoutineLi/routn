/*************************************************************************
	> File Name: Scheduler.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月31日 星期一 11时25分01秒
 ************************************************************************/

#include "Scheduler.h"
#include "Log.h"
#include "Hook.h"
#include "Macro.h"

namespace Routn
{
	static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

	static thread_local Schedular *t_schedular = nullptr;
	static thread_local Fiber *t_fiber = nullptr;

	Schedular::Schedular(size_t threads, bool use_caller, const std::string &name)
		: _name(name)
	{
		ROUTN_ASSERT(threads > 0);

		if (use_caller)
		{
			Routn::Fiber::GetThis();
			--threads;

			ROUTN_ASSERT(GetThis() == nullptr);
			t_schedular = this;

			_root_fiber.reset(NewFiber(std::bind(&Schedular::run, this), 0, true), FreeFiber);
			Routn::Thread::SetName(_name);

			t_fiber = _root_fiber.get();
			_root_thread = Routn::GetThreadId();
			_thread_id_vec.push_back(_root_thread);
		}
		else
		{
			_root_thread = -1;
		}
		_thread_cnt = threads;
	}

	Schedular::~Schedular()
	{
		ROUTN_ASSERT(_stopping);
		if (GetThis() == this)
		{
			t_schedular = nullptr;
		}
	}

	Schedular *Schedular::GetThis()
	{
		return t_schedular;
	}

	Fiber *Schedular::GetMainFiber()
	{
		return t_fiber;
	}

	void Schedular::start()
	{
		MutexType::Lock lock(_mutex);
		if (!_stopping)																	
		{
			return;
		}
		_stopping = false;
		ROUTN_ASSERT(_threads.empty());

		_threads.resize(_thread_cnt);
		for (size_t i = 0; i < _thread_cnt; ++i)
		{
			_threads[i].reset(new Thread(std::bind(&Schedular::run, this), _name + "_" + std::to_string(i)));
			_thread_id_vec.push_back(_threads[i]->getId());
		}
		lock.unlock();

		//if(_root_fiber){
		//	_root_fiber->call();
		//	ROUTN_LOG_INFO(g_logger) << "call out : " << _root_fiber->getStatus();
		//}
	}

	void Schedular::stop()
	{
		_autostop = true;
		if (_root_fiber 
			&& _thread_cnt == 0 
			&& (_root_fiber->getStatus() == Fiber::TERM 
				|| _root_fiber->getStatus() == Fiber::INIT))
		{
			ROUTN_LOG_INFO(g_logger) << this << " stopped";

			_stopping = true;
			if (stopping())
			{
				return;
			}
		}
		//bool exit_on_this_fiber = false;
		if (_root_thread != -1)
		{
			ROUTN_ASSERT(GetThis() == this);
		}
		else
		{
			ROUTN_ASSERT(GetThis() != this);
		}
		_stopping = true;
		for (size_t i = 0; i < _thread_cnt; ++i)
		{
			tickle();
		}

		if (_root_fiber)
		{
			tickle();
		}

		if (_root_fiber)
		{
			if(!stopping()) 
				_root_fiber->call();
		}

		std::vector<Thread::ptr> thrs;
		{
			MutexType::Lock lock(_mutex);
			thrs.swap(_threads);
		}

		for(auto& i : thrs) {
			i->join();
		}
	}

	void Schedular::idle(){
		ROUTN_LOG_INFO(g_logger) << "idle";
		while(!stopping()){
			Routn::Fiber::YieldToHold();
		}
	}


	void Schedular::run(){
		ROUTN_LOG_INFO(g_logger) << "run";
		set_hook_enable(true);
		setThis();
		if(Routn::GetThreadId() != _root_thread){
			t_fiber = Fiber::GetThis().get();
		}
		Fiber::ptr idle_fiber(NewFiber(std::bind(&Schedular::idle, this)), FreeFiber);
		Fiber::ptr cb_fiber;

		FiberAndThread ft;
		while(true){
			ft.reset();
			bool tickle_me = false;
			bool is_active = false;
			{
				MutexType::Lock lock(_mutex);
				auto it = _fibers.begin();
				while(it != _fibers.end()){
					if(it->thread_id != -1 
						&& it->thread_id != Routn::GetThreadId()){
							++it;
							tickle_me = true;
							continue;
					}

					ROUTN_ASSERT(it->fiber || it->callback);
					if(it->fiber && it->fiber->getStatus() == Fiber::EXEC){
						++it;
						continue;
					}

					ft = *it;
					_fibers.erase(it);
					++_active_thread_cnt;
					is_active = true;
					break;
				}
			}

			if(tickle_me){
				tickle();
			}

			if(ft.fiber && (ft.fiber->getStatus() != Fiber::TERM
							&& ft.fiber->getStatus() != Fiber::EXCEPT)){
				ft.fiber->swapIn();
				--_active_thread_cnt;

				if(ft.fiber->getStatus() == Fiber::READY){
					schedule(ft.fiber);
				}else if(ft.fiber->getStatus() != Fiber::TERM
						&& ft.fiber->getStatus() != Fiber::EXCEPT){
					ft.fiber->_fstatus = Fiber::HOLD;
				}
				ft.reset();
			}else if(ft.callback){
				if(cb_fiber){
					cb_fiber->reset(ft.callback);
				}else{
					cb_fiber.reset(new Fiber(ft.callback));
				}
				ft.reset();
				cb_fiber->swapIn();
				--_active_thread_cnt;
				if(cb_fiber->getStatus() == Fiber::READY){
					schedule(cb_fiber);
					cb_fiber.reset();
				}else if(cb_fiber->getStatus() == Fiber::EXCEPT
						|| cb_fiber->getStatus() == Fiber::TERM){
					cb_fiber->reset(nullptr);
				}else{
					cb_fiber->_fstatus = Fiber::HOLD;
					cb_fiber.reset();
				}
			}else{
				if(is_active){
					--_active_thread_cnt;
					continue;
				}
				if(idle_fiber->getStatus() == Fiber::TERM){
					ROUTN_LOG_INFO(g_logger) << "idle fiber term";
					break;
				}
				++_idle_thread_cnt;
				idle_fiber->swapIn();
				--_idle_thread_cnt;
				if(idle_fiber->getStatus() != Fiber::TERM
					&& idle_fiber->getStatus() != Fiber::EXCEPT){
					idle_fiber->_fstatus = Fiber::HOLD;
				}
			}
		}
		
	}

	bool Schedular::stopping(){
		MutexType::Lock lock(_mutex);
    	return _autostop && _stopping
        	&& _fibers.empty() && _active_thread_cnt == 0;
	}

	void Schedular::tickle()
	{
		ROUTN_LOG_INFO(g_logger) << "tickle";
	}

	void Schedular::setThis()
	{
		t_schedular = this;
	}

	void Schedular::switchTo(int thread){
		ROUTN_ASSERT(Schedular::GetThis() != nullptr);
		if(Schedular::GetThis() == this){
			if(thread == -1 || thread == Routn::GetThreadId()){
				return ;
			}
		}
		schedule(Fiber::GetThis(), thread);
		Fiber::YieldToHold();
	}

	SchedularSwitcher::SchedularSwitcher(Schedular* target){
		_caller = Schedular::GetThis();
		if(target){
			target->switchTo();
		}
	}

	SchedularSwitcher::~SchedularSwitcher(){
		if(_caller){
			_caller->switchTo();
		}
	}

}

/*************************************************************************
	> File Name: Scheduler.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月31日 星期一 10时59分38秒
 ************************************************************************/

#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <memory>
#include <list>
#include <vector>

#include "Fiber.h"
#include "Thread.h"

namespace Routn
{

	class Schedular
	{
	public:
		using ptr = std::shared_ptr<Schedular>;
		using MutexType = Mutex;

		Schedular(size_t threads = 1, bool use_caller = true, const std::string &name = "");
		virtual ~Schedular();

		const std::string &getName() const { return _name; }

		static Schedular *GetThis();
		static Fiber *GetMainFiber();

		void start();
		void stop();

		template <class FiberOrCallback>
		void schedule(FiberOrCallback fc, int thread = -1)
		{
			bool need_tickle = false;
			{
				MutexType::Lock lock(_mutex);
				need_tickle = scheduleNoLock(fc, thread);
			}
			if (need_tickle)
			{
				this->tickle();
			}
		}

		template <class InputIterator>
		void schedule(InputIterator begin, InputIterator end)
		{
			bool need_tickle = false;
			{
				MutexType::Lock lock(_mutex);
				while (begin != end)
				{
					need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
					++begin;
				}
			}
			if (need_tickle)
			{
				tickle();
			}
		}

		void switchTo(int thread = -1);

	protected:
		virtual void tickle();
		virtual bool stopping();
		void run();
		virtual void idle();
		void setThis();
	private:
		template <class FiberOrCallback>
		bool scheduleNoLock(FiberOrCallback fc, int thread)
		{
			bool need_tickle = _fibers.empty();
			FiberAndThread ft(fc, thread);
			if (ft.fiber || ft.callback)
			{
				_fibers.push_back(ft);
			}
			return need_tickle;
		}

	private:
		//协程队列封装的数据结构包括协程所在的线程id， 协程对象， 回调函数
		struct FiberAndThread
		{
			Fiber::ptr fiber;
			std::function<void()> callback;
			int thread_id;

			FiberAndThread(Fiber::ptr f, int id)
				: fiber(f), thread_id(id)
			{
			}
			FiberAndThread(Fiber::ptr *f, int id)
				: thread_id(id)
			{
				fiber.swap(*f);
			}
			FiberAndThread(std::function<void()> f, int id)
				: callback(f), thread_id(id)
			{
			}
			FiberAndThread(std::function<void()> *f, int id)
				: thread_id(id)
			{
				callback.swap(*f);
			}
			FiberAndThread()
			{
				thread_id = -1;
			}
			void reset()
			{
				fiber = nullptr;
				callback = nullptr;
				thread_id = -1;
			}
		};

	private:
		MutexType _mutex;				   //锁
		std::vector<Thread::ptr> _threads; //线程池
		std::list<FiberAndThread> _fibers; //协程队列
		Fiber::ptr _root_fiber;			   //主协程
		std::string _name;				   //调度器名称
	
	protected:
		std::vector<int> _thread_id_vec;   //线程id组
		size_t _thread_cnt = 0;				   //线程数量
		std::atomic<size_t> _active_thread_cnt = {0};		   //活跃线程数量
		std::atomic<size_t> _idle_thread_cnt = {0};		   //空闲线程数量
		bool _stopping = true;		   			   //停止标志位
		bool _autostop = false;		      		   //自动停止标志位
		int _root_thread = 0;				   //主线程id		
	};


	class SchedularSwitcher : public Noncopyable{
	public:
		SchedularSwitcher(Schedular* target = nullptr);
		~SchedularSwitcher();
	private:
		Schedular* _caller;
	};
}

#endif

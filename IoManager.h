/*************************************************************************
	> File Name: IoManager.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月01日 星期二 15时30分31秒
 ************************************************************************/

#ifndef _IOMANAGER_H
#define _IOMANAGER_H

#include "Scheduler.h"
#include "Timer.h"
#include <sys/epoll.h>

namespace Routn
{

	class IOManager : public Schedular, public TimerManager
	{
	public:
		using ptr = std::shared_ptr<IOManager>;
		using RWMutexType = RW_Mutex;

		enum Event
		{
			NONE = 0x0,
			READ = 0x1,
			WRITE = 0x4,
		};

	private:
		struct FdContext
		{
			using MutexType = Mutex;
			struct EventContext
			{
				Schedular *scheduler = nullptr; //事件执行的调度器
				Fiber::ptr fiber;				//事件协程
				std::function<void()> callback; //事件回调函数
			};

			EventContext& getContext(Event event);
			void resetContext(EventContext& ctx);
			void triggerEvent(Event event);

			int fd;				 //文件描述符
			EventContext read;	 //读事件
			EventContext write;	 //写事件
			Event events = NONE; //已经注册的事件
			MutexType mutex;	 //锁
		};

	public:
		IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "");
		~IOManager();
		int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
		bool delEvent(int fd, Event event);
		bool cancelEvent(int fd, Event event);

		bool cancelAll(int fd);

		static IOManager *GetThis();

		bool hasIdleThreads() { return _idle_thread_cnt > 0; } 
	protected:
		void tickle() override;
		bool stopping() override;
		void idle() override;
		void onTimerInsertedAtFront() override;

		bool stopping(uint64_t& timeout);
		void contextResize(size_t size);

	private:
		int _epollfd = 0;  //epollfd
		int _tickleFds[2]; //管道读写端fd数组

		std::atomic<size_t> _pendingEventCount = {0}; //等待执行的事件数量
		RWMutexType _mutex;
		std::vector<FdContext *> _fdContexts;
	};
}

#endif

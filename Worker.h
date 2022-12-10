/*************************************************************************
	> File Name: worker.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月28日 星期一 00时10分29秒
 ************************************************************************/

#ifndef _WORKER_H
#define _WORKER_H

#include "Noncopyable.h"
#include "FiberSem.h"
#include "Singleton.h"
#include "IoManager.h"
#include "Thread.h"
#include "Macro.h"
#include "Log.h"

#include <memory>
#include <vector>
#include <map>

namespace Routn{

	class WorkerGroup : Noncopyable, public std::enable_shared_from_this<WorkerGroup> {
	public:
		using ptr = std::shared_ptr<WorkerGroup>;
		static WorkerGroup::ptr Create(uint32_t batch_size, Routn::Schedular* s = Routn::Schedular::GetThis()) {
			return std::make_shared<WorkerGroup>(batch_size, s);
		}

		WorkerGroup(uint32_t batch_size, Routn::Schedular* s = Routn::Schedular::GetThis());
		~WorkerGroup();

		void schedule(std::function<void()> cb, int thread = -1);
		void schedule(const std::vector<std::function<void()> >& cbs);
		void waitAll();
	private:
		void doWork(std::function<void()> cb);
	private:
		uint32_t m_batchSize;
		bool m_finish;
		Schedular* m_scheduler;
		FiberSemaphore m_sem;
	};

	class TimedWorkerGroup : Noncopyable, public std::enable_shared_from_this<TimedWorkerGroup> {
	public:
		typedef std::shared_ptr<TimedWorkerGroup> ptr;
		static TimedWorkerGroup::ptr Create(uint32_t batch_size, uint32_t wait_ms, Routn::IOManager* s = Routn::IOManager::GetThis());

		TimedWorkerGroup(uint32_t batch_size, uint32_t wait_ms, Routn::IOManager* s = Routn::IOManager::GetThis());
		~TimedWorkerGroup();

		void schedule(std::function<void()> cb, int thread = -1);
		void waitAll();
	private:
		void doWork(std::function<void()> cb);
		void start();
		void onTimer();
	private:
		uint32_t m_batchSize;
		bool m_finish;
		bool m_timedout;
		uint32_t m_waitTime;
		Routn::Timer::ptr m_timer;
		IOManager* m_iomanager;
		FiberSemaphore m_sem;
	};

	class WorkerManager {
	public:
		WorkerManager();
		void add(Schedular::ptr s);
		Schedular::ptr get(const std::string& name);
		IOManager::ptr getAsIOManager(const std::string& name);

		template<class FiberOrCb>
		void schedule(const std::string& name, FiberOrCb fc, int thread = -1) {
			auto s = get(name);
			if(s) {
				s->schedule(fc, thread);
			} else {
				static Routn::Logger::ptr s_logger = ROUTN_LOG_NAME("system");
				ROUTN_LOG_ERROR(s_logger) << "schedule name=" << name
					<< " not exists";
			}
		}

		template<class Iter>
		void schedule(const std::string& name, Iter begin, Iter end) {
			auto s = get(name);
			if(s) {
				s->schedule(begin, end);
			} else {
				static Routn::Logger::ptr s_logger = ROUTN_LOG_NAME("system");
				ROUTN_LOG_ERROR(s_logger) << "schedule name=" << name
					<< " not exists";
			}
		}

		bool init();
		bool init(const std::map<std::string, std::map<std::string, std::string> >& v);
		void stop();

		bool isStoped() const { return m_stop;}
		std::ostream& dump(std::ostream& os);

		uint32_t getCount();
	private:
		void add(const std::string& name, Schedular::ptr s);
	private:
		std::map<std::string, std::vector<Schedular::ptr> > m_datas;
		bool m_stop;
	};

	using WorkerMgr = Singleton<WorkerManager>;

}

#endif

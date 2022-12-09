/*************************************************************************
	> File Name: Thread.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月29日 星期六 16时19分59秒
 ************************************************************************/

#include "Thread.h"
#include "Log.h"
#include "Fiber.h"
#include "Macro.h"
#include <utility>

namespace Routn
{

	static thread_local Thread *t_thread = nullptr;
	static thread_local std::string t_thread_name = "UNKNOWN";

	static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

	Semaphore::Semaphore(uint32_t count)
	{
		if (sem_init(&_sem, 0, count))
		{
			throw std::logic_error("sem_init error");
		}
	}

	Semaphore::~Semaphore()
	{
		sem_destroy(&_sem);
	}

	void Semaphore::notify()
	{
		if (sem_post(&_sem))
		{
			throw std::logic_error("sem_post error");
        }
	} //v operation +1

	void Semaphore::wait()
	{
		if (!sem_wait(&_sem))
		{
				return;
		}
	} //p operation -1

	Thread *Thread::GetThis()
	{
		return t_thread;
	}

	const std::string &Thread::GetName()
	{
		return t_thread_name;
	}

	void Thread::SetName(const std::string &name)
	{
		if (name.empty())
		{
			return;
		}
		if (t_thread)
		{
			t_thread->_name = name;
		}
		t_thread_name = name;
	}

	Thread::Thread(std::function<void()> cb, const std::string &name)
		: _callback(cb), _name(name)
	{
		if (name.empty())
		{
			_name = "UNKOWN";
		}
		int temp_thread = pthread_create(&_thread, nullptr, &Thread::run, this);
		if (temp_thread)
		{
			ROUTN_LOG_ERROR(g_logger) << "pthread_create thread fail, ret=" << temp_thread << " name=" << name;
			throw std::logic_error("pthread_create error");
		}
		_t_sem.wait();
	}

	Thread::~Thread()
	{
		if (_thread)
		{
			pthread_detach(_thread); //离开作用范围就强制让线程剥离
		}
	}

	void Thread::join()
	{
		if (_thread)
		{
			int ret = pthread_join(_thread, nullptr);
			if (ret)
			{
				ROUTN_LOG_ERROR(g_logger) << "pthread_join thread fail, ret=" << ret << " name=" << _name;
				throw std::logic_error("pthread_join error");
			}
			_thread = 0;
		}
	}

	void *Thread::run(void *arg)
	{
		Thread *thread = (Thread *)arg;
		t_thread = thread;
		t_thread_name = thread->_name;
		thread->_id = Routn::GetThreadId();
		pthread_setname_np(pthread_self(), thread->_name.substr(0, 15).c_str());
		std::function<void()> cb;
		cb.swap(thread->_callback); //当函数中存在智能指针时要防止其不能被释放掉

		thread->_t_sem.notify();

		cb(); //excute the callback

		return 0;
	}

}

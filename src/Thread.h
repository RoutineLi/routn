/*************************************************************************
	> File Name: Thread.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月29日 星期六 16时07分46秒
 ************************************************************************/

#pragma once

#include "Noncopyable.h"

#include <tbb/spin_rw_mutex.h>

#include <list>
#include <thread>
#include <iostream>
#include <functional>
#include <memory>
#include <atomic>
#include <semaphore.h>
#include <pthread.h>

using tid_t = pid_t;

namespace Routn
{

	class Semaphore
	{
	public:
		Semaphore(uint32_t count = 0);
		~Semaphore();

		void wait();   //p operation
		void notify(); //v operation
	private:
		Semaphore(const Semaphore &) = delete;
		Semaphore(const Semaphore &&) = delete;
		Semaphore &operator=(const Semaphore &) = delete;

	private:
		sem_t _sem;
	};

	//通用锁模板类
	template <class T>
	struct ScopedLockImpl
	{
		ScopedLockImpl(T &mutex)
			: _mutex(mutex)
		{
			_mutex.lock();
			_locked = true;
		}
		~ScopedLockImpl()
		{
			unlock();
		}
		void lock()
		{
			if (!_locked)
			{
				_mutex.lock();
				_locked = true;
			}
		}
		void unlock()
		{
			if (_locked)
			{
				_mutex.unlock();
				_locked = false;
			}
		}

	private:
		T &_mutex;
		bool _locked = false;
	};

	//读锁模板类
	template <class T>
	struct R_ScopedLockImpl
	{
		R_ScopedLockImpl(T &mutex)
			: _mutex(mutex)
		{
			_mutex.read_lock();
			_locked = true;
		}
		~R_ScopedLockImpl()
		{
			unlock();
		}
		void lock()
		{
			if (!_locked)
			{
				_mutex.read_lock();
				_locked = true;
			}
		}
		void unlock()
		{
			if (_locked)
			{
				_mutex.unlock();
				_locked = false;
			}
		}

	private:
		T &_mutex;
		bool _locked;
	};

	//写锁模板类
	template <class T>
	struct W_ScopedLockImpl
	{
		W_ScopedLockImpl(T &mutex)
			: _mutex(mutex)
		{
			_mutex.write_lock();
			_locked = true;
		}
		~W_ScopedLockImpl()
		{
			unlock();
		}
		void lock()
		{
			if (!_locked)
			{
				_mutex.write_lock();
				_locked = true;
			}
		}
		void unlock()
		{
			if (_locked)
			{
				_mutex.unlock();
				_locked = false;
			}
		}

	private:
		T &_mutex;
		bool _locked;
	};


	//空锁类：调试用
	class NullMutex
	{
	public:
		using Lock = ScopedLockImpl<NullMutex>;
		NullMutex() {}
		~NullMutex() {}
		void lock() {}
		void unlock() {}
	};


	//互斥锁类
	class Mutex
	{
	public:
		using Lock = ScopedLockImpl<Mutex>;
		Mutex()
		{
			pthread_mutex_init(&_mutex, nullptr);
		}
		~Mutex()
		{
			pthread_mutex_destroy(&_mutex);
		}
		void lock()
		{
			pthread_mutex_lock(&_mutex);
		}
		void trylock()
		{
			pthread_mutex_trylock(&_mutex);
		}
		void unlock()
		{
			pthread_mutex_unlock(&_mutex);
		}

	private:
		pthread_mutex_t _mutex;
	};

	//空读写锁类：调试使用
	class NullRWMutex
	{
	public:
		using ReadLock = R_ScopedLockImpl<NullMutex>;
		using WriteLock = W_ScopedLockImpl<NullMutex>;
		NullRWMutex() {}
		~NullRWMutex() {}
		void read_lock() {}
		void write_lock() {}
		void unlock() {}
	};

	//读写锁
	class RW_Mutex
	{
	public:
		using ReadLock = R_ScopedLockImpl<RW_Mutex>;
		using WriteLock = W_ScopedLockImpl<RW_Mutex>;

		RW_Mutex()
		{
			pthread_rwlock_init(&_lock, nullptr);
		}
		~RW_Mutex()
		{
			pthread_rwlock_destroy(&_lock);
		}
		void read_lock()
		{
			pthread_rwlock_rdlock(&_lock);
		}
		void write_lock()
		{
			pthread_rwlock_wrlock(&_lock);
		}
		void unlock()
		{
			pthread_rwlock_unlock(&_lock);
		}

	private:
		pthread_rwlock_t _lock;
	};

	//自旋锁 大并发情况下资源竞争很激烈，需要一种轻量级的锁
	class SpinLock
	{
	public:
		using Lock = ScopedLockImpl<SpinLock>;
		SpinLock()
		{
			pthread_spin_init(&_mutex, 0);
		}
		~SpinLock()
		{
			pthread_spin_destroy(&_mutex);
		}
		void lock()
		{
			pthread_spin_lock(&_mutex);
		}
		void unlock()
		{
			pthread_spin_unlock(&_mutex);
		}

	private:
		pthread_spinlock_t _mutex;
	};


	//读写自旋锁
	class RWSpinLock : public Noncopyable{
	public:
		using ReadLock = R_ScopedLockImpl<RWSpinLock>;
		using WriteLock = W_ScopedLockImpl<RWSpinLock>;
		RWSpinLock(){}
		~RWSpinLock(){}
		void write_lock(){
			_lock.lock();
			_isRead = false;
		}
		void read_lock(){
			_lock.lock_read();
			_isRead = true;
		}
		void unlock(){
			_lock.unlock();
		}
	private:
		tbb::spin_rw_mutex _lock;
		bool _isRead = false;
	};

	//CAS锁（原子锁）
	class CASLock
	{
	public:
		using Lock = ScopedLockImpl<CASLock> ;
		CASLock()
		{
			_mutex.clear();
		}
		~CASLock()
		{
		}
		void lock()
		{
			while (std::atomic_flag_test_and_set_explicit(&_mutex, std::memory_order_acquire))
				;
		}
		void unlock()
		{
			std::atomic_flag_clear_explicit(&_mutex, std::memory_order_release);
		}

	private:
		volatile std::atomic_flag _mutex;
	};

	//pthread线程封装类
	class Thread
	{
	public:
		using ptr = std::shared_ptr<Thread>;
		Thread(std::function<void()> cb, const std::string &name);
		~Thread();
		tid_t getId() const { return _id; }

		void join();

		static Thread *GetThis();
		static const std::string &GetName();
		static void SetName(const std::string &name);

	private:
		Thread(const Thread &) = delete;
		Thread(const Thread &&) = delete;
		Thread &operator=(const Thread &) = delete;
		static void *run(void *arg);

	private:
		tid_t _id = -1;
		pthread_t _thread = 0;
		std::function<void()> _callback;
		std::string _name;
		Semaphore _t_sem;
	};

}


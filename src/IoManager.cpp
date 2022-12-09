/*************************************************************************
	> File Name: IoManager.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月01日 星期二 15时53分37秒
 ************************************************************************/

#include "IoManager.h"
#include "Macro.h"
#include "Log.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

namespace Routn
{
	static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

	IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event)
	{
		switch (event)
		{
		case IOManager::READ:
			return read;
		case IOManager::WRITE:
			return write;
		default:
			ROUTN_ASSERT_ARG(false, "getContext");
		}
	}

	void IOManager::FdContext::resetContext(EventContext& ctx)
	{
		ctx.scheduler = nullptr;
		ctx.fiber.reset();
		ctx.callback = nullptr;	
	}

	void IOManager::FdContext::triggerEvent(Event event)
	{
		ROUTN_ASSERT(events & event);
		events = (Event)(events & ~event);
		EventContext& ctx = getContext(event);
		if(ctx.callback){
			ctx.scheduler->schedule(&ctx.callback);
		}else{
			ctx.scheduler->schedule(&ctx.fiber);
		}
		ctx.scheduler = nullptr;
		return;
	}
	

	IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
		: Schedular(threads, use_caller, name)
	{
		_epollfd = epoll_create(5000);
		ROUTN_ASSERT(_epollfd > 0);

		int ret = pipe(_tickleFds);
		ROUTN_ASSERT(!ret);

		//注册读事件
		epoll_event event;
		memset(&event, 0, sizeof(epoll_event));
		event.events = EPOLLIN | EPOLLET;
		event.data.fd = _tickleFds[0];
		///非阻塞读
		ret = fcntl(_tickleFds[0], F_SETFL, O_NONBLOCK);
		ROUTN_ASSERT(!ret);
		ret = epoll_ctl(_epollfd, EPOLL_CTL_ADD, _tickleFds[0], &event);
		ROUTN_ASSERT(!ret);

		contextResize(32);

		start();
	}

	IOManager::~IOManager()
	{
		stop();
		close(_epollfd);
		close(_tickleFds[0]);
		close(_tickleFds[1]);

		for (size_t i = 0; i < _fdContexts.size(); ++i)
		{
			if (_fdContexts[i])
			{
				delete _fdContexts[i];
			}
		}
	}

	int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
	{
		FdContext* temp_ctx = nullptr;
		RWMutexType::ReadLock lock(_mutex);
		if((int)_fdContexts.size() > fd)
		{
			temp_ctx = _fdContexts[fd];
			lock.unlock();
		}
		else
		{
			lock.unlock();
			RWMutexType::WriteLock lock_w(_mutex);
			contextResize(fd * 1.5);
			temp_ctx = _fdContexts[fd];
		}

		FdContext::MutexType::Lock lock_m(temp_ctx->mutex);
		if(ROUTN_UNLIKELY(temp_ctx->events & event))
		{
			ROUTN_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd
						<< " event = " << event
						<< " temp_ctx.event = " << temp_ctx->events;
			ROUTN_ASSERT(!(temp_ctx->events & event));
		}

		int op = temp_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
		epoll_event _event;
		_event.events = EPOLLET | temp_ctx->events | event;
		_event.data.ptr = temp_ctx;

		int ret = epoll_ctl(_epollfd, op, fd, &_event);
		if(ret)
		{
			ROUTN_LOG_ERROR(g_logger) << "epoll_ctl(" << _epollfd << ", "
				<< op << ", " << fd << ", " << _event.events << ") : "
				<< ret << " (" << errno << ") (" << strerror(errno) << ")";
			return -1;
		}

		++_pendingEventCount;
		temp_ctx->events = (Event)(temp_ctx->events | event);
		FdContext::EventContext& event_ctx = temp_ctx->getContext(event);
		ROUTN_ASSERT(!event_ctx.scheduler
			&& !event_ctx.fiber
			&& !event_ctx.callback);
		
		event_ctx.scheduler = Schedular::GetThis();
		if(cb){
			event_ctx.callback.swap(cb);
		}else{
			event_ctx.fiber = Fiber::GetThis();
			ROUTN_ASSERT_ARG(event_ctx.fiber->getStatus() == Fiber::EXEC
							, "status = " << event_ctx.fiber->getStatus());
		}

		return 0;
	}

	bool IOManager::delEvent(int fd, Event event)
	{
		RWMutexType::ReadLock lock(_mutex);
		if((int_least32_t)_fdContexts.size() <= fd)
		{
			return false;
		}
		FdContext* fd_ctx = _fdContexts[fd];
		lock.unlock();

		FdContext::MutexType::Lock lock_m(fd_ctx->mutex);
		if((!fd_ctx->events) & event){
			return false;
		}

		Event new_events = (Event)(fd_ctx->events & ~event);
		int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
		epoll_event _event;
		_event.events = EPOLLET | new_events;
		_event.data.ptr = fd_ctx;

		int ret = epoll_ctl(_epollfd, op, fd, &_event);

		if(ret){
			ROUTN_LOG_ERROR(g_logger) << "epoll_ctl(" << _epollfd << ", "
				<< op << ", " << fd << ", " << _event.events << ") : "
				<< ret << " (" << errno << ") (" << strerror(errno) << ")";
			return false;			
		}

		--_pendingEventCount;
		fd_ctx->events = new_events;
		FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
		fd_ctx->resetContext(event_ctx);
		return true;
	}

	bool IOManager::cancelEvent(int fd, Event event)
	{	
		RWMutexType::ReadLock lock(_mutex);
		if(_fdContexts.size() <= (size_t)fd)
		{
			return false;
		}
		FdContext* fd_ctx = _fdContexts[fd];
		lock.unlock();

		FdContext::MutexType::Lock lock_m(fd_ctx->mutex);
		if((!fd_ctx->events) & event){
			return false;
		}

		Event new_events = (Event)(fd_ctx->events & ~event);
		int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
		epoll_event _event;
		_event.events = EPOLLET | new_events;
		_event.data.ptr = fd_ctx;

		int ret = epoll_ctl(_epollfd, op, fd, &_event);

		if(ret){
			ROUTN_LOG_ERROR(g_logger) << "epoll_ctl(" << _epollfd << ", "
				<< op << ", " << fd << ", " << _event.events << ") : "
				<< ret << " (" << errno << ") (" << strerror(errno) << ")";
			return false;			
		}

		fd_ctx->triggerEvent(event);
		--_pendingEventCount;
		return true;
	}

	bool IOManager::cancelAll(int fd)
	{
		RWMutexType::ReadLock lock(_mutex);
		if(_fdContexts.size() <= (size_t)fd)
		{
			return false;
		}
		FdContext* fd_ctx = _fdContexts[fd];
		lock.unlock();

		FdContext::MutexType::Lock lock_m(fd_ctx->mutex);
		if(!fd_ctx->events){
			return false;
		}

		
		int op =  EPOLL_CTL_DEL;
		epoll_event _event;
		_event.events = 0;
		_event.data.ptr = fd_ctx;

		int ret = epoll_ctl(_epollfd, op, fd, &_event);

		if(ret){
			ROUTN_LOG_ERROR(g_logger) << "epoll_ctl(" << _epollfd << ", "
				<< op << ", " << fd << ", " << _event.events << ") : "
				<< ret << " (" << errno << ") (" << strerror(errno) << ")";
			return false;			
		}

		if(fd_ctx->events & READ)
		{
			fd_ctx->triggerEvent(READ);
			--_pendingEventCount;
		}
		if(fd_ctx->events & WRITE)
		{
			fd_ctx->triggerEvent(WRITE);
			--_pendingEventCount;
		}
		ROUTN_ASSERT(fd_ctx->events == 0);
		return true;
	}

	void IOManager::contextResize(size_t size)
	{
		_fdContexts.resize(size);

		for(size_t i = 0; i < _fdContexts.size(); ++i)
		{
			if(!_fdContexts[i]){
				_fdContexts[i] = new FdContext;
				_fdContexts[i]->fd = i;
			}
		}
	}

	IOManager *IOManager::GetThis()
	{
		return dynamic_cast<IOManager*>(Schedular::GetThis());
	}


	void IOManager::tickle(){
		if(!hasIdleThreads()){
			return ;
		}
		int ret = write(_tickleFds[1], "T", 1);
		ROUTN_ASSERT(ret == 1);
	}

	bool IOManager::stopping(){
		uint64_t timeout = 0;
		return stopping(timeout);
	}

	void IOManager::idle(){
		epoll_event* events = new epoll_event[64]();
		std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
			delete[] ptr;
		});

		while(true){
			uint64_t next_timeout = 0;
			if(stopping(next_timeout)){
				ROUTN_LOG_INFO(g_logger) << "name = " 
										 << getName() 
										 << " idle stopping exit";
				break;
			}

			int ret = 0;
			do{
				static const int MAX_TIMEOUT = 3000;
				if(next_timeout != ~0ull){
					next_timeout = (int)next_timeout > MAX_TIMEOUT 
									? MAX_TIMEOUT : next_timeout;
				}else{
					next_timeout = MAX_TIMEOUT;
				}
				ret = epoll_wait(_epollfd, events, 64, (int)next_timeout);

	            if(ret < 0 && errno == EINTR) {
            	} else {
                	break;
				}
			}while(true);
				
			std::vector<std::function<void()> > cbs;
			listExpiredCb(cbs);
			if(!cbs.empty()){
				schedule(cbs.begin(), cbs.end());
				cbs.clear();
			}
			for(int i = 0; i < ret; ++i){
				epoll_event& event = events[i];
				if(event.data.fd == _tickleFds[0]){
					uint8_t dummy;
					while(read(_tickleFds[0], &dummy, 1) == 1);
					continue;
				}

				FdContext* fd_ctx = (FdContext*)event.data.ptr;
				FdContext::MutexType::Lock lock(fd_ctx->mutex);
				if(event.events & (EPOLLERR | EPOLLHUP)){
					event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
				}
				int real_events = NONE;
				if(event.events & EPOLLIN){
					real_events |= READ;
				}
				if(event.events & EPOLLOUT){
					real_events |= WRITE;
				}

				if((fd_ctx->events & real_events) == NONE){
					continue;
				}

				int left_events = (fd_ctx->events & ~real_events);
				int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
				event.events = EPOLLET | left_events;

				int ret2 = epoll_ctl(_epollfd, op, fd_ctx->fd, &event);
				if(ret2){
					ROUTN_LOG_ERROR(g_logger) << "epoll_ctl(" << _epollfd << ", "
						<< op << ", " << fd_ctx->fd << ", " << event.events << ") : "
						<< ret << " (" << errno << ") (" << strerror(errno) << ")";	
					continue;
				}

				if(real_events & READ){
					fd_ctx->triggerEvent(READ);
					--_pendingEventCount;
				}
				if(real_events & WRITE){
					fd_ctx->triggerEvent(WRITE);
					--_pendingEventCount;
				}
			}

			Fiber::ptr cur = Fiber::GetThis();
			auto naked_ptr = cur.get();
			cur.reset();

			naked_ptr->swapOut();
		}
	}

	void IOManager::onTimerInsertedAtFront(){
		tickle();
	}

	bool IOManager::stopping(uint64_t& timeout){
		timeout = getNextTimer();
		return timeout == ~0ull
				&& _pendingEventCount == 0
				&& Schedular::stopping();
	}
}

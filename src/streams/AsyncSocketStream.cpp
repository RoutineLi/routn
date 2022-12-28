/*************************************************************************
	> File Name: AsyncSocketStream.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月28日 星期三 00时10分08秒
 ************************************************************************/

#include "AsyncSocketStream.h"
#include "../Util.h"
#include "../Log.h"
#include "../Macro.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn{

	AsyncSocketStream::AsyncSocketStream(Socket::ptr sock, bool owner)
		: SocketStream(sock, owner)
		, _waitSem(2)
		, _sn(0)
		, _autoConnect(false)
		, _ioMgr(nullptr)
		, _worker(nullptr) {

	}

	AsyncSocketStream::Ctx::Ctx()
		: sn(0)
		, timeout(0)
		, result(0)
		, timed(false)
		, scheduler(nullptr) {

	}

	void AsyncSocketStream::Ctx::doRsp(){
		Schedular* t = scheduler;
		if(!Routn::Atomic::compareAndSwapBool(scheduler, t, (Schedular*)nullptr)){
			return ;
		}
		if(!t || !fiber){
			return ;
		}
		if(timer){
			timer->cancel();
			timer = nullptr;
		}
		if(timed){
			result = TIMEOUT;
		}
		t->schedule(&fiber);
	}
	
	bool AsyncSocketStream::start(){
		if(!_ioMgr){
			_ioMgr = Routn::IOManager::GetThis();
		}
		if(!_worker){
			_worker = Routn::IOManager::GetThis();
		}

		do{
			waitFiber();
			if(_timer){
				_timer->cancel();
				_timer = nullptr;
			}
			if(!isConnected()){
				if(!_socket->reconnect()){
					innerClose();
					_waitSem.notify();
					_waitSem.notify();
					break;
				}
			}

			if(_connectCB){
				if(!_connectCB(shared_from_this())){
					innerClose();
					_waitSem.notify();
					_waitSem.notify();
					break;
				}
			}

			startRead();
			startWrite();
			return true;
		}while(false);

		if(_autoConnect){
			if(_timer){
				_timer->cancel();
				_timer = nullptr;
			}

			_timer = _ioMgr->addTimer(2 * 1000, std::bind(&AsyncSocketStream::start, shared_from_this()));
		}
		return false;
	}
	
	void AsyncSocketStream::close(){
		_autoConnect = false;
		SchedularSwitcher ss(_ioMgr);
		if(_timer){
			_timer->cancel();
		}
		SocketStream::close();
	}

	void AsyncSocketStream::doRead(){
		try{
			while(isConnected()){
				auto ctx = doRecv();
				if(ctx){
					ctx->doRsp();
				}
			}
		}catch(...){

		}
		ROUTN_LOG_DEBUG(g_logger) << "doRead out " << this;
		innerClose();
		_waitSem.notify();

		if(_autoConnect){
			_ioMgr->addTimer(10, std::bind(&AsyncSocketStream::start, shared_from_this()));
		}
	}

	void AsyncSocketStream::doWrite(){
		try{
			while(isConnected()){
				_sem.wait();
				std::list<SendCtx::ptr> ctxs;
				{
					RWMutexType::WriteLock lock(_queueMutex);
					_queue.swap(ctxs);
				}
				auto self = shared_from_this();
				for(auto& i : ctxs){
					if(!i->doSend(self)){
						innerClose();
						break;
					}
				}
			}
		}catch(...){

		}
		ROUTN_LOG_DEBUG(g_logger) << "doWrite out " << this;
		{
			RWMutexType::WriteLock lock(_queueMutex);
			_queue.clear();
		}
		_waitSem.notify();
	}

	void AsyncSocketStream::startRead(){
		_ioMgr->schedule(std::bind(&AsyncSocketStream::doRead, shared_from_this()));
	}

	void AsyncSocketStream::startWrite(){	
		_ioMgr->schedule(std::bind(&AsyncSocketStream::doWrite, shared_from_this()));
	}

	void AsyncSocketStream::onTimeOut(Ctx::ptr ctx){
		{
			RWMutexType::WriteLock lock(_mutex);
			_ctxs.erase(ctx->sn);
		}
		ctx->timed = true;
		ctx->doRsp();
	}


	AsyncSocketStream::Ctx::ptr AsyncSocketStream::getCtx(uint32_t sn){
		RWMutexType::ReadLock lock(_mutex);
		auto it = _ctxs.find(sn);
		return it != _ctxs.end() ? it->second : nullptr;
	}

	AsyncSocketStream::Ctx::ptr AsyncSocketStream::getAndDelCtx(uint32_t sn){
		Ctx::ptr ctx;
		RWMutexType::WriteLock lock(_mutex);
		auto it = _ctxs.find(sn);
		if(it != _ctxs.end()){
			ctx = it->second;
			_ctxs.erase(it);
		}
		return ctx;
	}


	bool AsyncSocketStream::addCtx(Ctx::ptr ctx){
		RWMutexType::WriteLock lock(_mutex);
		_ctxs.emplace(ctx->sn, ctx);
		return true;
	}

	bool AsyncSocketStream::enqueue(SendCtx::ptr ctx){
		ROUTN_ASSERT(ctx);
		RWMutexType::WriteLock lock(_queueMutex);
		bool empty = _queue.empty();
		_queue.push_back(ctx);
		lock.unlock();
		if(empty){
			_sem.notify();
		}
		return empty;
	}


	bool AsyncSocketStream::innerClose(){
		ROUTN_ASSERT(_ioMgr == Routn::IOManager::GetThis());
		if(isConnected() && _disconnectCB){
			_disconnectCB(shared_from_this());
		}
		SocketStream::close();
		_sem.notify();
		std::unordered_map<uint32_t, Ctx::ptr> ctxs;
		{
			RWMutexType::WriteLock lock(_mutex);
			ctxs.swap(_ctxs);
		}
		{
			RWMutexType::WriteLock lock(_queueMutex);
			_queue.clear();
		}
		for(auto& i : ctxs){
			i.second->result = IO_ERROR;
			i.second->doRsp();
		}
		return true;
	}

	bool AsyncSocketStream::waitFiber(){
		_waitSem.wait();
		_waitSem.wait();
		return true;
	}



	AsyncSocketStreamManager::AsyncSocketStreamManager()
		: _size(0)
		, _idx(0){

	}

	void AsyncSocketStreamManager::add(AsyncSocketStream::ptr stream){
		RWMutexType::WriteLock lock(_mutex);
		_datas.push_back(stream);
		++_size;

		if(_conncect_callback){
			stream->setConnectCB(_conncect_callback);
		}

		if(_disconnect_callback){
			stream->setDisConnectCB(_disconnect_callback);
		}

	}

	void AsyncSocketStreamManager::clear(){
		RWMutexType::WriteLock lock(_mutex);
		for(auto& i : _datas){
			i->close();
		}
		_datas.clear();
		_size = 0;
	}

	void AsyncSocketStreamManager::setConnection(const std::vector<AsyncSocketStream::ptr>& streams){
		auto cs = streams;
		RWMutexType::WriteLock lock(_mutex);
		cs.swap(_datas);
		_size = _datas.size();
		if(_conncect_callback || _disconnect_callback){
			for(auto& i : _datas){
				if(_conncect_callback)
					i->setConnectCB(_conncect_callback);
				if(_disconnect_callback)
					i->setDisConnectCB(_disconnect_callback);
			}
		}
		lock.unlock();

		for(auto& i: cs){
			i->close();
		}
	}

	AsyncSocketStream::ptr AsyncSocketStreamManager::get(){
		RWMutexType::ReadLock lock(_mutex);
		for(uint32_t i = 0; i < _size; ++i){
			auto idx = Atomic::addFetch(_idx, 1);
			if(_datas[idx % _size]->isConnected()){
				return _datas[idx % _size];
			}
		}
		return nullptr;
	}

	void AsyncSocketStreamManager::setConnectCB(connect_callback v){
		_conncect_callback = v;
		RWMutexType::WriteLock lock(_mutex);
		for(auto &i : _datas){
			i->setConnectCB(_conncect_callback);
		}
	}

	void AsyncSocketStreamManager::setDisConnectCB(disconnect_callback v){
		_disconnect_callback = v;
		RWMutexType::WriteLock lock(_mutex);
		for(auto &i : _datas){
			i->setDisConnectCB(_disconnect_callback);
		}		
	}
	

}


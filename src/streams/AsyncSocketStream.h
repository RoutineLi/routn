/*************************************************************************
	> File Name: AsyncSocketStream.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月28日 星期三 00时10分02秒
 ************************************************************************/

#ifndef _ASYNCSOCKETSTREAM_H
#define _ASYNCSOCKETSTREAM_H
 
#include "SocketStream.h"
#include "../FiberSync.h"
#include "../IoManager.h"
#include "../Thread.h"
#include "../Timer.h"
#include <list>
#include <unordered_map>
#include <boost/any.hpp>

namespace Routn{

class AsyncSocketStream : public SocketStream
						, public std::enable_shared_from_this<AsyncSocketStream>{
public:
	using ptr = std::shared_ptr<AsyncSocketStream>;
	using RWMutexType = Routn::RW_Mutex;
	using connect_callback = std::function<bool(AsyncSocketStream::ptr)> ;
	using disconnect_callback = std::function<void(AsyncSocketStream::ptr)> ;

	AsyncSocketStream(Socket::ptr sock, bool owner = true);

	virtual bool start();
	virtual void close() override;
public:
	enum Error{
		OK = 0,
		TIMEOUT = -1,
		IO_ERROR = -2,
		NOT_CONNECT = -3
	};
protected:
	struct SendCtx{
	public:
		using ptr = std::shared_ptr<SendCtx>;
		virtual ~SendCtx(){}
		virtual bool doSend(AsyncSocketStream::ptr stream) = 0;
	};

	struct Ctx : public SendCtx{
	public:
		using ptr = std::shared_ptr<Ctx>;
		virtual ~Ctx(){}
		Ctx();

		uint32_t sn;
		uint32_t timeout;
		uint32_t result;
		bool timed;

		Schedular* scheduler;
		Fiber::ptr fiber;
		Timer::ptr timer;

		virtual void doRsp();
	};

public:
	void setWorker(Routn::IOManager* v) { _worker = v;}
	IOManager* getWorker() const { return _worker;}

	void setIOManager(IOManager* v) { _ioMgr = v;}
	IOManager* getIOManager() const { return _ioMgr;}

	bool isAutoConnect() const { return _autoConnect;}
	void setAutoConnect(bool v) { _autoConnect = v;}

	connect_callback getConnectCB() const { return _connectCB;}
	disconnect_callback getDisConnectCB() const { return _disconnectCB;} 
	void setConnectCB(connect_callback v) { _connectCB = v;}
	void setDisConnectCB(disconnect_callback v) { _disconnectCB = v;}

	template<class T>
	void setData(const T& v){ _data = v;}

	template<class T>
	T getData() const {
		try{
			return boost::any_cast<T>(_data);
		}catch(...){

		}
		return T();
	}

protected:
	virtual void doRead();
	virtual void doWrite();
	virtual void startRead();
	virtual void startWrite();
	virtual void onTimeOut(Ctx::ptr ctx);
	virtual Ctx::ptr doRecv() = 0;

	Ctx::ptr getCtx(uint32_t sn);
	Ctx::ptr getAndDelCtx(uint32_t sn);

	template<class T>
	std::shared_ptr<T> getCtxAs(uint32_t sn){
		auto ctx = getCtx(sn);
		if(ctx){
			return std::dynamic_pointer_cast<T>(ctx);
		}
		return nullptr;
	}

	template<class T>
	std::shared_ptr<T> getAndDelCtxAs(uint32_t sn){
		auto ctx = getAndDelCtx(sn);
		if(ctx){
			return std::dynamic_pointer_cast<T>(ctx);
		}
		return nullptr;
	}

	bool addCtx(Ctx::ptr ctx);
	bool enqueue(SendCtx::ptr ctx);

	bool innerClose();
	bool waitFiber();
protected:
	Routn::FiberSemaphore _sem;
	Routn::FiberSemaphore _waitSem;
	RWMutexType _queueMutex;
	std::list<SendCtx::ptr> _queue;
	RWMutexType _mutex;
	std::unordered_map<uint32_t, Ctx::ptr> _ctxs;
	uint32_t _sn;
	bool _autoConnect;
	Routn::Timer::ptr _timer;
	Routn::IOManager* _ioMgr;
	Routn::IOManager* _worker;

	connect_callback _connectCB;
	disconnect_callback _disconnectCB;

	boost::any _data;
};

}

#endif

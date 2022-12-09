/*************************************************************************
	> File Name: FdManager.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月03日 星期四 19时16分45秒
 ************************************************************************/

#include "FdManager.h"

namespace Routn
{

	void FdCtx::setTimeout(int type, uint64_t v)
	{
		if(type == SO_RCVTIMEO || type == SO_RCVTIMEO_NEW){
			_recvTimeout = v;
		}else{
			_sendTimeout = v;
		}
	}

	uint64_t FdCtx::getTimeout(int type)
	{
		if(type == SO_RCVTIMEO || type == SO_RCVTIMEO_NEW){
			return _recvTimeout;
		}else{
			return _sendTimeout;
		}
	}

	FdCtx::FdCtx(int fd)
		: _isInit(false)
		, _isSocket(false)
		, _sysNonblock(false)
		, _userNonblock(false)
		, _isClosed(false)
		, _fd(fd)
		, _recvTimeout(-1)
		, _sendTimeout(-1)
	{
		init();
	}

	FdCtx::~FdCtx()
	{
	}

	bool FdCtx::init()
	{
		if (_isInit)
		{
			return true;
		}
		_recvTimeout = -1;
		_sendTimeout = -1;

		struct stat fd_stat;
		if (-1 == fstat(_fd, &fd_stat))
		{
			_isInit = false;
			_isSocket = false;
		}
		else
		{
			_isInit = true;
			_isSocket = S_ISSOCK(fd_stat.st_mode);
		}

		if (_isSocket)
		{
			int flags = fcntl_f(_fd, F_GETFL, 0);
			if (!(flags & O_NONBLOCK))
			{
				fcntl_f(_fd, F_SETFL, flags | O_NONBLOCK);
			}
			_sysNonblock = true;
		}
		else
		{
			_sysNonblock = false;
		}

		_userNonblock = false;
		_isClosed = false;
		return _isInit;
	}

	FdCtx::ptr FdManager::get(int fd, bool auto_create)
	{
		if(fd == -1){
			return nullptr;
		}
		RWMutexType::ReadLock lock(_mutex);
		if((int)_datas.size() <= fd){
			if(auto_create == false){
				return nullptr;
			}
		}else{
			if(_datas[fd] || !auto_create){
				return _datas[fd];
			}
		}
		lock.unlock();

		RWMutexType::WriteLock lock2(_mutex);
		FdCtx::ptr ctx(new FdCtx(fd));
		if(fd >= (int)_datas.size()){
			_datas.resize(fd * 1.5);
		}
		_datas[fd] = ctx;
		return ctx;
	}

	void FdManager::del(int fd)
	{
		RWMutexType::WriteLock lock(_mutex);
		if((int)_datas.size() <= fd){
			return ;
		}
		_datas[fd].reset();
	}

	FdManager::FdManager()
	{
		_datas.resize(64);
	}

}
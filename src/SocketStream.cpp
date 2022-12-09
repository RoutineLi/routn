/*************************************************************************
	> File Name: SocketStream.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月21日 星期一 08时05分55秒
 ************************************************************************/

#include "SocketStream.h"

namespace Routn{
	SocketStream::SocketStream(Socket::ptr sock, bool owner)
		: _socket(sock)
		, _owner(owner){
	}
	
	SocketStream::~SocketStream(){
		if(_owner && _socket){
			_socket->close();
		}
	}

	bool SocketStream::isConnected() const{
		return _socket && _socket->isConnected();
	}

	int SocketStream::read(void *buffer, size_t length){
		if(!isConnected()) return -1;
		return _socket->recv(buffer, length);
	}

	int SocketStream::read(ByteArray::ptr ba, size_t length){
		if(!isConnected()) return -1;
		std::vector<iovec> iovs;
		ba->getWriteBuffers(iovs, length);
		int ret = _socket->recv(&iovs[0], iovs.size());
		if(ret > 0){
			ba->setPosition(ba->getPosition() + ret);
		}
		return ret;
	}	
	
	int SocketStream::write(const void *buffer, size_t length){
		if(!isConnected()) return -1;
		return _socket->send(buffer, length);
	}

	int SocketStream::write(ByteArray::ptr ba, size_t length){
		if(!isConnected()) return -1;
		std::vector<iovec> iovs;
		ba->getReadBuffers(iovs, length);
		int ret = _socket->recv(&iovs[0], iovs.size());
		if(ret > 0){
			ba->setPosition(ba->getPosition() + ret);
		}
		return ret;
	}

	void SocketStream::close(){
		if(_socket){
			_socket->close();
		}
	}

}


/*************************************************************************
	> File Name: SocketStream.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月21日 星期一 08时02分36秒
 ************************************************************************/

#ifndef _SOCKETSTREAM_H
#define _SOCKETSTREAM_H

#include "Stream.h"
#include "Socket.h"

namespace Routn{

	class SocketStream : public Stream{
	public:
		using ptr = std::shared_ptr<SocketStream>;
		SocketStream(Socket::ptr sock, bool owner = true);
		~SocketStream();
		int read(void *buffer, size_t length) override;
		int read(ByteArray::ptr ba, size_t length) override;
		
		int write(const void *buffer, size_t length) override;
		int write(ByteArray::ptr ba, size_t length) override;

		void close() override;
		bool isConnected() const;

		Socket::ptr getSocket() const { return _socket;}
	protected:
		Socket::ptr _socket;
		bool _owner;
	};

}

#endif

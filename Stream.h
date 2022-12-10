/*************************************************************************
	> File Name: Stream.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月21日 星期一 07时52分22秒
 ************************************************************************/

#ifndef _STREAM_H
#define _STREAM_H

#include <memory>
#include "ByteArray.h"

namespace Routn{

	class Stream{
	public:
		using ptr = std::shared_ptr<Stream>;
		virtual ~Stream();
		Stream();

		virtual int read(void *buffer, size_t length) = 0;
		virtual int read(ByteArray::ptr ba, size_t length) = 0;
		virtual int readFixSize(void *buffer, size_t length);
		virtual int readFixSize(ByteArray::ptr ba, size_t length);
		
		virtual int write(const void *buffer, size_t length) = 0;
		virtual int write(ByteArray::ptr ba, size_t length) = 0;
		virtual int writeFixSize(const void *buffer, size_t length);
		virtual int writeFixSize(ByteArray::ptr ba, size_t length);

		virtual void close() = 0;
	};
}

#endif

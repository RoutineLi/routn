/*************************************************************************
	> File Name: ZLIBStream.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月27日 星期二 22时23分54秒
 ************************************************************************/

#ifndef _ZLIBSTREAM_H
#define _ZLIBSTREAM_H

#include "Stream.h"
#include <zlib.h>
#include <sys/uio.h>
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace Routn{

	class ZLIBStream : public Stream{
	public:
		using ptr = std::shared_ptr<ZLIBStream>;
		enum Type{
			ZLIB,
			DEFLATE,
			GZIP
		};

		enum Strategy{
			DEFAULT = Z_DEFAULT_STRATEGY,
			FILTERED = Z_FILTERED,
			HUFFMAN = Z_HUFFMAN_ONLY,
			FIXED = Z_FIXED,
			RLE = Z_RLE
		};

		enum CompressLevel{
			NO_COMPRESSION = Z_NO_COMPRESSION, 
			BEST_SPEED = Z_BEST_SPEED,
			BEST_COMPRESSION = Z_BEST_COMPRESSION,
			DEFAULT_COMPRESSION = Z_DEFAULT_COMPRESSION
		};

		static ZLIBStream::ptr CreateGzip(bool encode, uint32_t buff_size = 4096);
		static ZLIBStream::ptr CreateZlib(bool encode, uint32_t buff_size = 4096);
		static ZLIBStream::ptr CreateDeflate(bool encode, uint32_t buff_size = 4096);
		static ZLIBStream::ptr Create(bool encode, uint32_t buff_size = 4096
			, Type type = DEFLATE, int level = DEFAULT_COMPRESSION, int window_bits = 15
			, int memlevel = 8, Strategy strategy = DEFAULT);
	
		ZLIBStream(bool encode, uint32_t buff_size = 4096);
		~ZLIBStream();

		virtual int read(void* buffer, size_t length) override;
		virtual int read(ByteArray::ptr ba, size_t length) override;
		virtual int write(const void* buffer, size_t length) override;
		virtual int write(ByteArray::ptr ba, size_t length) override;
		virtual void close() override;

		int flush();

		bool isFree() const { return _free;}
		void setFree(bool v) { _free = v;}

		bool isEncode() const { return _encode;}
		void setEncode(bool v) { _encode = v;}

		std::vector<iovec>& getBuffers() { return _buffs;}
		std::string getResult() const;
		Routn::ByteArray::ptr getByteArray();
	private:
		int init(Type type = DEFLATE, int level = DEFAULT_COMPRESSION, int window_bits = 15, int memlevel = 8, Strategy Strategy = DEFAULT);

		int encode(const iovec* v, const uint64_t& size, bool finish);
		int decode(const iovec* v, const uint64_t& size, bool finish);
	private:
		z_stream _zstream;
		uint32_t _buffsize;
		bool _encode;
		bool _free;
		std::vector<iovec> _buffs;
	};
}


#endif

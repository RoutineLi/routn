	/*************************************************************************
	> File Name: ZLIBStream.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月27日 星期二 22时24分00秒
 ************************************************************************/


#include "ZLIBStream.h"
#include "../Macro.h"

namespace Routn{


		ZLIBStream::ptr ZLIBStream::CreateGzip(bool encode, uint32_t buff_size){
			return Create(encode, buff_size, GZIP);
		}

		ZLIBStream::ptr ZLIBStream::CreateZlib(bool encode, uint32_t buff_size){
			return Create(encode, buff_size, ZLIB);
		}

		ZLIBStream::ptr ZLIBStream::CreateDeflate(bool encode, uint32_t buff_size){
			return Create(encode, buff_size, DEFLATE);
		}

		ZLIBStream::ptr ZLIBStream::Create(bool encode, uint32_t buff_size
			, Type type, int level, int window_bits
			, int memlevel, Strategy strategy){
			
			ZLIBStream::ptr rt(new ZLIBStream(encode, buff_size));
			if(rt->init(type, level, window_bits, memlevel, strategy) == Z_OK){
				return rt;
			}
			return nullptr;
		}
	
		ZLIBStream::ZLIBStream(bool encode, uint32_t buff_size)
			: _buffsize(buff_size)
			, _encode(encode)
			, _free(true) {

		}

		ZLIBStream::~ZLIBStream(){
			if(_free){
				for(auto& i : _buffs){
					free(i.iov_base);
				}
			}

			if(_encode){
				deflateEnd(&_zstream);
			}else{
				inflateEnd(&_zstream);
			}
		}

		int ZLIBStream::read(void* buffer, size_t length){
			throw std::logic_error("ZLIBStream::read is invalid");
		}

		int ZLIBStream::read(ByteArray::ptr ba, size_t length){
			throw std::logic_error("ZLIBStream::read is invalid");
		}

		int ZLIBStream::write(const void* buffer, size_t length){
			iovec iov;
			iov.iov_base = (void*)buffer;
			iov.iov_len = length;
			if(_encode){
				return encode(&iov, 1, false);
			}else{
				return decode(&iov, 1, false);
			}
		}

		int ZLIBStream::write(ByteArray::ptr ba, size_t length){
			std::vector<iovec> buffers;
			ba->getReadBuffers(buffers, length);
			if(_encode){
				return encode(&buffers[0], buffers.size(), false);
			}else{
				return decode(&buffers[0], buffers.size(), false);
			}
		}

		void ZLIBStream::close(){
			flush();
		}

		int ZLIBStream::flush(){
			iovec iov;
			iov.iov_base = nullptr;
			iov.iov_len = 0;

			if(_encode){
				return encode(&iov, 1, true);
			}else{
				return decode(&iov, 1, true);
			}
		}

		std::string ZLIBStream::getResult() const{
			std::string rt;
			for(auto& i : _buffs){
				rt.append((const char*)i.iov_base, i.iov_len);
			}
			return rt;
		}

		Routn::ByteArray::ptr ZLIBStream::getByteArray(){
			ByteArray::ptr ba(new ByteArray);
			for(auto& i : _buffs){
				ba->write(i.iov_base, i.iov_len);
			}
			ba->setPosition(0);
			return ba;
		}


		int ZLIBStream::init(Type type, int level, int window_bits, int memlevel, Strategy Strategy){
			ROUTN_ASSERT((level >= 0 && level <= 9) || level == DEFAULT_COMPRESSION);
			ROUTN_ASSERT((window_bits >= 8 && window_bits <= 15));
			ROUTN_ASSERT((memlevel >= 1 && memlevel <= 9));

			memset(&_zstream, 0, sizeof(_zstream));

			_zstream.zalloc = Z_NULL;
			_zstream.zfree = Z_NULL;
			_zstream.opaque = Z_NULL;

			switch (type)
			{
			case DEFLATE:
				window_bits = -window_bits;
				break;
			case GZIP:
				window_bits += 16;
				break;
			case ZLIB:
			default:
				break;
			}

			if(_encode){
				return deflateInit2(&_zstream, level, Z_DEFLATED, window_bits, memlevel, (int)Strategy);
			}else{
				return inflateInit2(&_zstream, window_bits);
			}
		}

		int ZLIBStream::encode(const iovec* v, const uint64_t& size, bool finish){
			int ret = 0;
			int flush = 0;
			for(uint64_t i = 0; i < size; ++i){
				_zstream.avail_in = v[i].iov_len;
				_zstream.next_in = (Bytef*)v[i].iov_base;

				flush = finish ? (i == size - 1 ? Z_FINISH : Z_NO_FLUSH) : Z_NO_FLUSH;

				iovec* iov = nullptr;
				do{
					if(!_buffs.empty() && _buffs.back().iov_len != _buffsize){
						iov = &_buffs.back();
					}else{
						iovec t;
						t.iov_base = malloc(_buffsize);
						t.iov_len = 0;
						_buffs.push_back(t);
						iov = &_buffs.back();
					}

					_zstream.avail_out = _buffsize - iov->iov_len;
					_zstream.next_out = (Bytef*)iov->iov_base + iov->iov_len;

					ret = deflate(&_zstream, flush);
					if(ret == Z_STREAM_ERROR){
						return ret;
					}
					iov->iov_len = _buffsize - _zstream.avail_out;
				}while(_zstream.avail_out == 0);
			}
			if(flush == Z_FINISH){
				deflateEnd(&_zstream);
			}
			return Z_OK;
		}

		int ZLIBStream::decode(const iovec* v, const uint64_t& size, bool finish){
			int ret = 0;
			int flush = 0;
			for(uint64_t i = 0; i < size; ++i){
				_zstream.avail_in = v[i].iov_len;
				_zstream.next_in = (Bytef*)v[i].iov_base;

				flush = finish ? (i == size - 1 ? Z_FINISH : Z_NO_FLUSH) : Z_NO_FLUSH;

				iovec* iov = nullptr;
				do{
					if(!_buffs.empty() && _buffs.back().iov_len != _buffsize){
						iov = &_buffs.back();
					}else{
						iovec t;
						t.iov_base = malloc(_buffsize);
						t.iov_len = 0;
						_buffs.push_back(t);
						iov = &_buffs.back();
					}

					_zstream.avail_out = _buffsize - iov->iov_len;
					_zstream.next_out = (Bytef*)iov->iov_base + iov->iov_len;

					ret = inflate(&_zstream, flush);
					if(ret == Z_STREAM_ERROR){
						return ret;
					}
					iov->iov_len = _buffsize - _zstream.avail_out;
				}while(_zstream.avail_out == 0);
			}
			if(flush == Z_FINISH){
				inflateEnd(&_zstream);
			}
			return Z_OK;
		}

}

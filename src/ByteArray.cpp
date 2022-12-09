/*************************************************************************
	> File Name: src/ByteArray.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月07日 星期一 17时52分48秒
 ************************************************************************/

#include "ByteArray.h"
#include "Endian.h"
#include "Log.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn
{

	ByteArray::Node::Node(::size_t s)
		: ptr(new char[s]), size(s), next(nullptr)
	{
	}

	ByteArray::Node::Node()
		: ptr(nullptr), size(0), next(nullptr)
	{
	}

	ByteArray::Node::~Node()
	{
		if (ptr)
		{
			delete[] ptr;
		}
	}

	ByteArray::ByteArray(size_t base_size)
		: _baseSize(base_size)
		, _capacity(base_size)
		, _position(0)
		, _size(0)
		, _endian(ROUTN_BIG_ENDIAN)
		, _root(new Node(base_size))
		, _cur(_root)
	{
	}
	ByteArray::~ByteArray()
	{
		Node *temp = _root;
		while (temp)
		{
			_cur = temp;
			temp = temp->next;
			delete _cur;
		}
	}

	bool ByteArray::isLittleEndian() const
	{
		return _endian == ROUTN_LITTLE_ENDIAN;
	}

	void ByteArray::setIsLittleEndian(bool val)
	{
		if (val)
		{
			_endian = ROUTN_LITTLE_ENDIAN;
		}
		else
		{
			_endian = ROUTN_BIG_ENDIAN;
		}
	}

	//write
	void ByteArray::writeFint8(int8_t value)
	{
		write(&value, sizeof(value));
	}

	void ByteArray::writeFuint8(uint8_t value)
	{
		write(&value, sizeof(value));
	}

	void ByteArray::writeFint16(int16_t value)
	{
		if (_endian != ROUTN_BYTE_ORDER)
		{
			value = byteswap(value);
		}
		write(&value, sizeof(value));
	}

	void ByteArray::writeFuint16(uint16_t value)
	{
		if (_endian != ROUTN_BYTE_ORDER)
		{
			value = byteswap(value);
		}
		write(&value, sizeof(value));
	}

	void ByteArray::writeFint32(int32_t value)
	{
		if (_endian != ROUTN_BYTE_ORDER)
		{
			value = byteswap(value);
		}
		write(&value, sizeof(value));
	}

	void ByteArray::writeFuint32(uint32_t value)
	{
		if (_endian != ROUTN_BYTE_ORDER)
		{
			value = byteswap(value);
		}
		write(&value, sizeof(value));
	}

	void ByteArray::writeFint64(int64_t value)
	{
		if (_endian != ROUTN_BYTE_ORDER)
		{
			value = byteswap(value);
		}
		write(&value, sizeof(value));
	}

	void ByteArray::writeFuint64(uint64_t value)
	{
		if (_endian != ROUTN_BYTE_ORDER)
		{
			value = byteswap(value);
		}
		write(&value, sizeof(value));
	}

	static uint32_t EncodeZigzag32(const int32_t &v)
	{
		if (v < 0)
		{
			return ((uint32_t)(-v)) * 2 - 1;
		}
		else
		{
			return v * 2;
		}
	}

	static uint32_t EncodeZigzag64(const int64_t &v)
	{
		if (v < 0)
		{
			return ((uint64_t)(-v)) * 2 - 1;
		}
		else
		{
			return v * 2;
		}
	}

	static int32_t DecodeZigzag32(const uint32_t &v)
	{
		return (v >> 1) ^ -(v & 1);
	}

	static int64_t DecodeZigzag64(const uint64_t &v)
	{
		return (v >> 1) ^ -(v & 1);
	}

	void ByteArray::writeInt32(int32_t value)
	{
		writeUint32(EncodeZigzag32(value));
	}

	void ByteArray::writeUint32(uint32_t value)
	{
		uint8_t tmp[5];
		uint8_t i = 0;
		while (value >= 0x80)
		{
			tmp[i++] = (value & 0x7f) | 0x80;
			value >>= 7;
		}
		tmp[i++] = value;
		write(tmp, i);
	}

	void ByteArray::writeInt64(int64_t value)
	{
		writeUint64(EncodeZigzag64(value));
	}

	void ByteArray::writeUint64(uint64_t value)
	{
		uint8_t tmp[10];
		uint8_t i = 0;
		while (value >= 0x80)
		{
			tmp[i++] = (value & 0x7f) | 0x80;
			value >>= 7;
		}
		tmp[i++] = value;
		write(tmp, i);
	}

	void ByteArray::writeFloat(float value)
	{
		uint32_t v;
		memcpy(&v, &value, sizeof(value));
		writeFuint32(v);
	}

	void ByteArray::writeDouble(double value)
	{
		uint64_t v;
		memcpy(&v, &value, sizeof(value));
		writeFuint64(v);
	}

	void ByteArray::writeStringF16(const std::string &value)
	{
		writeFuint16(value.size());
		write(value.c_str(), value.size());
	}

	void ByteArray::writeStringF32(const std::string &value)
	{
		writeFuint32(value.size());
		write(value.c_str(), value.size());
	}

	void ByteArray::writeStringF64(const std::string &value)
	{
		writeFuint64(value.size());
		write(value.c_str(), value.size());
	}

	void ByteArray::writeStringVint(const std::string &value)
	{
		writeUint64(value.size());
		write(value.c_str(), value.size());
	}

	void ByteArray::writeStringWithoutLength(const std::string &value)
	{
		write(value.c_str(), value.size());
	}

	//read
	int8_t ByteArray::readFint8()
	{
		int8_t v;
		read(&v, sizeof(v));
		return v;
	}

	uint8_t ByteArray::readFuint8()
	{
		uint8_t v;
		read(&v, sizeof(v));
		return v;
	}

#define XX(type)                     \
	type v;                          \
	read(&v, sizeof(v));             \
	if (_endian == ROUTN_BYTE_ORDER) \
	{                                \
		return v;                    \
	}                                \
	return byteswap(v);

	int16_t ByteArray::readFint16()
	{
		XX(int16_t);
	}

	uint16_t ByteArray::readFuint16()
	{
		XX(uint16_t);
	}

	int32_t ByteArray::readFint32()
	{
		XX(int32_t);
	}

	uint32_t ByteArray::readFuint32()
	{
		XX(uint32_t);
	}

	int64_t ByteArray::readFint64()
	{
		XX(int64_t);
	}

	uint64_t ByteArray::readFuint64()
	{
		XX(uint64_t);
	}

#undef XX

	int32_t ByteArray::readInt32()
	{
		return DecodeZigzag32(readUint32());
	}

	uint32_t ByteArray::readUint32()
	{
		uint32_t res = 0;
		for (int i = 0; i < 32; i += 7)
		{
			uint8_t b = readFuint8();
			if (b < 0x80)
			{
				res |= ((uint32_t)b) << i;
				break;
			}
			else
			{
				res |= (((uint32_t)(b & 0x7f)) << i);
			}
		}
		return res;
	}

	int64_t ByteArray::readInt64()
	{
		return DecodeZigzag64(readUint64());
	}

	uint64_t ByteArray::readUint64()
	{
		uint64_t res = 0;
		for (int i = 0; i < 64; i += 7)
		{
			uint8_t b = readFuint8();
			if (b < 0x80)
			{
				res |= ((uint64_t)b) << i;
				break;
			}
			else
			{
				res |= (((uint64_t)(b & 0x7f)) << i);
			}
		}
		return res;
	}

	float ByteArray::readFloat()
	{
		uint32_t v = readFuint32();
		float value;
		memcpy(&value, &v, sizeof(v));
		return value;
	}

	double ByteArray::readDouble()
	{
		uint64_t v = readFuint64();
		double value;
		memcpy(&value, &v, sizeof(v));
		return value;
	}

	std::string ByteArray::readStringF16()
	{
		uint16_t len = readFuint16();
		std::string buff;
		buff.resize(len);
		read(&buff[0], len);
		return buff;
	}

	std::string ByteArray::readStringF32()
	{
		uint32_t len = readFuint32();
		std::string buff;
		buff.resize(len);
		read(&buff[0], len);
		return buff;
	}

	std::string ByteArray::readStringF64()
	{
		uint64_t len = readFuint64();
		std::string buff;
		buff.resize(len);
		read(&buff[0], len);
		return buff;
	}

	std::string ByteArray::readStringVint()
	{
		uint64_t len = readUint64();
		std::string buff;
		buff.resize(len);
		read(&buff[0], len);
		return buff;
	}

	//basic func
	void ByteArray::clear()
	{
		_position = _size = 0;
		_capacity = _baseSize;
		Node *tmp = _root->next;
		while (tmp)
		{
			_cur = tmp;
			tmp = tmp->next;
			delete _cur;
		}
		_cur = _root;
		_root->next = NULL;
	}

	void ByteArray::write(const void *buf, size_t size)
	{
		if (size == 0)
		{
			return;
		}
		addCapacity(size);

		size_t npos = _position % _baseSize;
		size_t ncap = _cur->size - npos;
		size_t bpos = 0;

		while (size > 0)
		{
			if (ncap >= size)
			{
				memcpy(_cur->ptr + npos, (const char*)buf + bpos, size);
				if (_cur->size == (npos + size))
				{
					_cur = _cur->next;
				}
				_position += size;
				bpos += size;
				size = 0;
			}
			else
			{
				memcpy(_cur->ptr + npos, (const char*)buf + bpos, ncap);
				_position += ncap;
				bpos += ncap;
				size -= ncap;
				_cur = _cur->next;
				ncap = _cur->size;
				npos = 0;
			}
		}

		if (_position > _size)
		{
			_size = _position;
		}
	}

	void ByteArray::read(void *buf, size_t size)
	{
		if (size > getReadSize())
		{
			throw std::out_of_range("not enouth length");
		}

		size_t npos = _position % _baseSize;
		size_t ncap = _cur->size - npos;
		size_t bpos = 0;
		while (size > 0)
		{
			if (ncap >= size)
			{
				memcpy((char*)buf + bpos, _cur->ptr + npos, size);
				if (_cur->size == (npos + size))
				{
					_cur = _cur->next;
				}
				_position += size;
				bpos += size;
				size = 0;
			}
			else
			{
				memcpy((char*)buf + bpos, _cur->ptr + npos, ncap);
				_position += ncap;
				bpos += ncap;
				size -= ncap;
				_cur = _cur->next;
				ncap = _cur->size;
				npos = 0;
			}
		}
	}

	void ByteArray::Read(void *buf, size_t size, size_t position) const
	{
		if (size > getReadSize())
		{
			throw std::out_of_range("not enouth length");
		}

		size_t npos = position % _baseSize;
		size_t ncap = _cur->size - npos;
		size_t bpos = 0;
		Node *cur = _cur;
		while (size > 0)
		{
			if (ncap >= size)
			{
				memcpy((char*)buf + bpos, cur->ptr + npos, size);
				if (cur->size == (npos + size))
				{
					cur = cur->next;
				}
				position += size;
				bpos += size;
				size = 0;
			}
			else
			{
				memcpy((char*)buf + bpos, _cur->ptr + npos, ncap);
				position += ncap;
				bpos += ncap;
				size -= ncap;
				cur = cur->next;
				ncap = cur->size;
				npos = 0;
			}
		}
	}

	void ByteArray::setPosition(size_t v)
	{
		if (v > _capacity)
		{
			throw std::out_of_range("set position out of range");
		}
		_position = v;
		if(_position > _size){
			_size = _position;
		}
		_cur = _root;
		while (v > _cur->size)
		{
			v -= _cur->size;
			_cur = _cur->next;
		}
		if (v == _cur->size)
		{
			_cur = _cur->next;
		}
	}

	bool ByteArray::writeToFile(const std::string &name) const
	{
		std::ofstream ofs;
		ofs.open(name, std::ios::trunc | std::ios::binary);
		if (!ofs)
		{
			ROUTN_LOG_ERROR(g_logger) << "writeToFile name = " << name
									  << " error, errno = " << errno << " reason: " << strerror(errno);
			return false;
		}

		int64_t read_size = getReadSize();
		int64_t pos = _position;
		Node *cur = _cur;

		while (read_size > 0)
		{
			int diff = pos % _baseSize;
			int64_t len = (read_size > (int64_t)_baseSize ? _baseSize : read_size) - diff;
			ofs.write(cur->ptr + diff, len);
			cur = cur->next;
			pos += len;
			read_size -= len;
		}

		return true;
	}

	bool ByteArray::readFrom(const std::string &name)
	{
		std::ifstream ifs;
		ifs.open(name, std::ios::binary);
		if (!ifs)
		{
			ROUTN_LOG_ERROR(g_logger) << "read from " << name
									  << " error, errno = " << errno << " reason: "
									  << strerror(errno);
			return false;
		}

		std::shared_ptr<char> buffer(new char[_baseSize], [](char *ptr)
									 { delete[] ptr; });
		while (!ifs.eof())
		{
			ifs.read(buffer.get(), _baseSize);
			write(buffer.get(), ifs.gcount());
		}
		return true;
	}

	void ByteArray::addCapacity(size_t size)
	{
		if (size == 0)
			return;
		if (getCapcity() > size)
		{
			return;
		}
		size_t old_cap = getCapcity();
		if (old_cap >= size)
		{
			return;
		}

		size = size - old_cap;
		size_t count = ceil(1.0 * size / _baseSize);
		Node *tmp = _root;
		while (tmp->next)
		{
			tmp = tmp->next;
		}

		Node *first = NULL;
		for (size_t i = 0; i < count; ++i)
		{
			tmp->next = new Node(_baseSize);
			if (first == NULL)
			{
				first = tmp->next;
			}
			tmp = tmp->next;
			_capacity += _baseSize;
		}

		if (old_cap == 0)
		{
			_cur = first;
		}
	}

	std::string ByteArray::toString() const
	{
		std::string str;
		str.resize(getReadSize());
		if (str.empty())
		{
			return str;
		}
		Read(&str[0], str.size(), _position);
		return str;
	}

	std::string ByteArray::toHexString() const
	{
		std::string str = toString();
		std::stringstream ss;

		for (size_t i = 0; i < str.size(); ++i)
		{
			if (i > 0 && (i % 32) == 0)
			{
				ss << std::endl;
			}
			ss << std::setw(2) << std::setfill('0') << std::hex
			   << (int)(uint8_t)str[i] << " ";
		}
		return ss.str();
	}

	//从当前位置
	uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len)
	{
		len = len > getReadSize() ? getReadSize() : len;
		if (len == 0)
		{
			return 0;
		}

		uint64_t size = len;

		size_t npos = _position % _baseSize;
		size_t ncap = _cur->size - npos;
		struct iovec iov;
		Node *cur = _cur;

		while (len > 0)
		{
			if (ncap >= len)
			{
				iov.iov_base = cur->ptr + npos;
				iov.iov_len = len;
				len = 0;
			}
			else
			{
				iov.iov_base = cur->ptr + npos;
				iov.iov_len = ncap;
				len -= ncap;
				cur = cur->next;
				ncap = cur->size;
				npos = 0;
			}
			buffers.push_back(iov);
		}
		return size;
	}

	//从指定位置
	uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len, uint64_t position) const
	{
		len = len > getReadSize() ? getReadSize() : len;
		if (len == 0)
		{
			return 0;
		}

		uint64_t size = len;

		size_t npos = position % _baseSize;
		size_t count = position / _baseSize;

		Node *cur = _root;
		while (count > 0)
		{
			cur = cur->next;
			--count;
		}

		size_t ncap = cur->size - npos;
		struct iovec iov;

		while (len > 0)
		{
			if (ncap >= len)
			{
				iov.iov_base = cur->ptr + npos;
				iov.iov_len = len;
				len = 0;
			}
			else
			{
				iov.iov_base = cur->ptr + npos;
				iov.iov_len = ncap;
				len -= ncap;
				cur = cur->next;
				ncap = cur->size;
				npos = 0;
			}
			buffers.push_back(iov);
		}
		return size;
	}

	uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len)
	{
		if(len == 0){
			return 0;
		}
		addCapacity(len);
		uint64_t size = len;


		size_t npos = _position % _baseSize;
		size_t ncap = _cur->size - npos;
		struct iovec iov;
		Node* cur = _cur;
		while(len > 0){
			if(ncap >= len){
				iov.iov_base = cur->ptr + npos;
				iov.iov_len = len;
				len = 0;
			}else{
				iov.iov_base = cur->ptr + npos;
				iov.iov_len = ncap;

				len -= ncap;
				cur = cur->next;
				ncap = cur->size;
				npos = 0;
			}
			buffers.push_back(iov);
		}
		return size;
	}

}

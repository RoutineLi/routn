/*************************************************************************
	> File Name: src/ByteArray.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月07日 星期一 17时31分46秒
 ************************************************************************/

#ifndef _BYTE_ARRAY_H
#define _BYTE_ARRAY_H

#include <memory>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <sys/socket.h>
#include <sstream>
#include <vector>
#include <iomanip>
#include <string>
#include <cstring>

namespace Routn{

	class ByteArray{
	public:
		using ptr = std::shared_ptr<ByteArray>;

		struct Node{
			Node(size_t s);
			Node();
			~Node();
			char* ptr;
			size_t size;
			Node* next;
		};
		//size = page size
		ByteArray(size_t base_size = 4096);
		~ByteArray();

		//write
		void writeFint8(int8_t value);
		void writeFuint8(uint8_t value);
		void writeFint16(int16_t value);
		void writeFuint16(uint16_t value);
		void writeFint32(int32_t value);
		void writeFuint32(uint32_t value);
		void writeFint64(int64_t value);
		void writeFuint64(uint64_t value);

		void writeInt32(int32_t value);
		void writeUint32(uint32_t value);
		void writeInt64(int64_t value);
		void writeUint64(uint64_t value);

		void writeFloat(float value);
		void writeDouble(double value);

		void writeStringF16(const std::string& value);
		void writeStringF32(const std::string& value);
		void writeStringF64(const std::string& value);
		void writeStringVint(const std::string& value);
		void writeStringWithoutLength(const std::string& value);


		//read
		///定长度的读
		int8_t readFint8();
		uint8_t readFuint8();
		int16_t readFint16();
		uint16_t readFuint16();
		int32_t readFint32();
		uint32_t readFuint32();
		int64_t readFint64();
		uint64_t readFuint64();
		///不定长度读
		int32_t readInt32();
		uint32_t readUint32();
		int64_t readInt64();
		uint64_t readUint64();

		float readFloat();
		double readDouble();

		std::string readStringF16();
		std::string readStringF32();
		std::string readStringF64();
		std::string readStringVint();

		//basic func
		void clear();

		void write(const void* buf, size_t size);
		void read(void* buf, size_t size);
		void Read(void* buf, size_t size, size_t position) const;

		size_t getPosition() const {return _position; }
		void setPosition(size_t v);

		bool writeToFile(const std::string& name) const;
		bool readFrom(const std::string& name);
		size_t getbaseSize() const {return _baseSize; }
		size_t getReadSize() const {return _size - _position; }
	
		bool isLittleEndian() const;
		void setIsLittleEndian(bool val);

		std::string toString() const;
		std::string toHexString() const;
		
		//只获取内容，不修改position
		uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull);
		uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
		//增加容量，不修改position
		uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);
		size_t getSize() const{ return _size; }
	private:
		void addCapacity(size_t size);
		size_t getCapcity() const {return _capacity - _position;}
	private:
		size_t _baseSize;
		size_t _capacity;
		size_t _position;
		size_t _size;
		int8_t _endian;
		Node* _root;
		Node* _cur;
	};

}

#endif

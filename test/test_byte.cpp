/*************************************************************************
	> File Name: test/test_byte.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月08日 星期二 12时48分34秒
 ************************************************************************/

#include "../src/ByteArray.h"
#include "../src/Routn.h"
#include <ctime>

static Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

void test(){
#define XX(type, len, write_fun, read_fun, base_len) {\
	std::vector<type> vec; \
	for(int i = 0; i < len; ++i){	\
		vec.push_back(rand());	\
	}	\
	Routn::ByteArray::ptr ba(new Routn::ByteArray(base_len)); 	\
	for(auto& i : vec){	\
		ba->write_fun(i);	\
	}	\
	ba->setPosition(0);	\
	for(size_t i = 0; i < vec.size(); ++i){	\
		int32_t v = ba->read_fun();	\
		ROUTN_ASSERT(v == vec[i]);	\
	}	\
	ROUTN_ASSERT(ba->getReadSize() == 0);	\
	ROUTN_LOG_INFO(g_logger) << #write_fun << "/" #read_fun << " (" <<  #type << ") len = " << #len	\
			<< " base_len = " << #base_len << " size = " << ba->getSize();	\
}

	XX(int8_t, 100, writeFint8, readFint8, 1);
	XX(uint8_t, 100, writeFuint8, readFuint8, 1);
	XX(int16_t, 100, writeFint16, readFint16, 1);
	XX(uint16_t, 100, writeFuint16, readFuint16, 1);
#undef XX

#define XX(type, len, write_fun, read_fun, base_len) {\
	std::vector<type> vec; \
	for(int i = 0; i < len; ++i){	\
		vec.push_back(rand());	\
	}	\
	Routn::ByteArray::ptr ba(new Routn::ByteArray(base_len)); 	\
	for(auto& i : vec){	\
		ba->write_fun(i);	\
	}	\
	ba->setPosition(0);	\
	for(size_t i = 0; i < vec.size(); ++i){	\
		type v = ba->read_fun();	\
		ROUTN_ASSERT(v == vec[i]);	\
	}	\
	ROUTN_ASSERT(ba->getReadSize() == 0);	\
	ROUTN_LOG_INFO(g_logger) << #write_fun << "/" #read_fun << " (" <<  #type << ") len = " << #len	\
			<< " base_len = " << #base_len << " size = " << ba->getSize();	\
	ba->setPosition(0); 	\
	ROUTN_ASSERT(ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat"));	\
	Routn::ByteArray::ptr ba2(new Routn::ByteArray(base_len));		\
	ROUTN_ASSERT(ba2->readFrom("/tmp/" #type "_" #len "-" #read_fun ".dat"));	\
	ba2->setPosition(0);	\
	ROUTN_ASSERT(ba->toString() == ba2->toString());	\
	ROUTN_ASSERT(ba->getPosition() == 0);	\
	ROUTN_ASSERT(ba2->getPosition() == 0);	\
}
	std::cout << "-------------------------------" << std::endl;
    XX(int8_t,  100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t,  100, writeFint16,  readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t,  100, writeFint32,  readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t,  100, writeFint64,  readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t,  100, writeInt32,  readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t,  100, writeInt64,  readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);
#undef XX
}

int main(int argc, char** argv){
	test();
	return 0;
}


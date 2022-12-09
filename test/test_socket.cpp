/*************************************************************************
	> File Name: ../test/test_socket.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月07日 星期一 16时44分34秒
 ************************************************************************/

#include "../src/Socket.h"
#include "../src/Routn.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

void test(){
	Routn::IPAddress::ptr addr = Routn::Address::LookupAnyIPAddress("www.baidu.com", AF_UNSPEC);
	if(addr){
		ROUTN_LOG_INFO(g_logger) << "get addr: " << addr->toString();
	}else{
		ROUTN_LOG_ERROR(g_logger) << "get addr fail";
		return;
	}

	Routn::Socket::ptr sock = Routn::Socket::CreateTCP(addr);
	addr->setPort(80);
	if(!sock->connect(addr)){
		ROUTN_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
		return ;
	}else{
		ROUTN_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
	}

	const char buff[] = "GET / HTTP/1.0\r\n\r\n";
	int ret = sock->send(buff, sizeof(buff));
	if(ret <= 0){
		ROUTN_LOG_ERROR(g_logger) << "send fail ret = " << ret;
		return ;
	}

	std::string buffer;
	buffer.resize(4096);
	ret = sock->recv(&buffer[0], buffer.size());

	if(ret <= 0){
		ROUTN_LOG_ERROR(g_logger) << "recv fail ret = " << ret;
		return ;
	}

	buffer.resize(ret);
	ROUTN_LOG_INFO(g_logger) << buffer;
}

int main(int argc, char** argv){
	Routn::IOManager iom;
	iom.schedule(&test);
	return 0;
}


/*************************************************************************
	> File Name: test_Tcp_server.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月20日 星期日 21时49分27秒
 ************************************************************************/

#include "../src/Routn.h"
#include "../src/TcpServer.h"

Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

void test(){
	auto addr = Routn::Address::LookupAny("172.20.10.3:8033", AF_INET);
	auto addr2 = Routn::UnixAddress::ptr(new Routn::UnixAddress("/tmp/unix_addr"));
	ROUTN_LOG_INFO(g_logger) << *addr << " - " << *addr2;
	std::vector<Routn::Address::ptr> addrs;
	addrs.push_back(addr);
	addrs.push_back(addr2);

	Routn::TcpServer::ptr tcp_server(new Routn::TcpServer);
	std::vector<Routn::Address::ptr> fails;
	while(!tcp_server->bind(addrs, fails)){
		sleep(2);
	}
	tcp_server->start();
}

int main(int argc, char** argv){
	Routn::IOManager iom(2);
	iom.schedule(test);
	return 0;
}



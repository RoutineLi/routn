/*************************************************************************
	> File Name: test_http_connection.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月23日 星期三 18时48分11秒
 ************************************************************************/

#include <iostream>
#include "../src/Routn.h"
#include "../src/http/HttpConnection.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

void test2();

void test(){
	Routn::IPAddress::ptr addr = Routn::Address::LookupAnyIPAddress("www.baidu.com");
	if(!addr){
		ROUTN_LOG_ERROR(g_logger) << "get addr error";
		return ;
	}
	ROUTN_LOG_INFO(g_logger) << "get addr: " << *addr;

	Routn::Socket::ptr sock = Routn::Socket::CreateTCP(addr);
	addr->setPort(80);
	bool ret = sock->connect(addr);
	if(!ret){
		ROUTN_LOG_ERROR(g_logger) << "connect " << *addr << " failed!";
		return ;
	}
	Routn::Http::HttpConnection::ptr conn(new Routn::Http::HttpConnection(sock));
	Routn::Http::HttpRequest::ptr req(new Routn::Http::HttpRequest);
	req->setHeader("host", "www.baidu.com");
	req->setPath("/duty/");
	ROUTN_LOG_INFO(g_logger) << "req: " << std::endl << req->toString();
	conn->sendRequest(req);
	auto rsp = conn->recvResponse();
	if(!rsp){
		ROUTN_LOG_INFO(g_logger) << "no response";
		return ;
	}
	ROUTN_LOG_INFO(g_logger) << "rsp: " << std::endl << rsp->toString();
	
	ROUTN_LOG_INFO(g_logger) << "==================================================================";
	auto ret2 = Routn::Http::HttpConnection::DoGet("http://www.baidu.com", 300);
	
	ROUTN_LOG_INFO(g_logger) << "result = " << ret2->result
		<< " error = " << ret2->error
		<< " rsp = " << (ret2 ? ret2->response->toString() : "");
	
	ROUTN_LOG_INFO(g_logger) << "==================================================================";
	test2();
}

//connection-pool
void test2(){
	Routn::Http::HttpConnectionPool::ptr pool(new Routn::Http::HttpConnectionPool("www.baidu.com", "", 80, 10, 1000 * 30, 5));
	Routn::IOManager::GetThis()->addTimer(1000, [pool](){
		auto r = pool->doGet("/", 300);
		ROUTN_LOG_INFO(g_logger) << r->toString();
	}, true);
}

int main(int argc, char** argv){
	Routn::IOManager iom(2);
	iom.schedule(test2);
	return 0;
}


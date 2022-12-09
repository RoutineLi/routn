/*************************************************************************
	> File Name: test_hook.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月03日 星期四 18时05分01秒
 ************************************************************************/

#include "../src/Routn.h"
#include <arpa/inet.h>

Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

void test_sleep(){
	Routn::IOManager iom(1);

	iom.schedule([](){
		sleep(1);
		ROUTN_LOG_INFO(g_logger) << "sleep 2";
	});

	iom.schedule([](){
		sleep(2);
		ROUTN_LOG_INFO(g_logger) << "sleep 3";
	});
	ROUTN_LOG_INFO(g_logger) << "test_sleep";
}

void test_sock(){
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	inet_pton(AF_INET, "116.177.224.239", &addr.sin_addr.s_addr);

	ROUTN_LOG_INFO(g_logger) << "begin connect ";
	
	int ret = connect(sock, (const struct sockaddr*)&addr, sizeof(addr));
	
	ROUTN_LOG_INFO(g_logger) << "connect ret = " << ret << " errno = " << errno;

	if(ret){
		return ;
	}

	const char data[] = "GET / HTTP/1.1\r\n\r\n";
	ret = send(sock, data, sizeof(data), 0);
	ROUTN_LOG_INFO(g_logger) << "send ret = " << ret << " errno = " << errno;

	if(ret <= 0){
		return ;
	}

	std::string buff;
	buff.resize(4096);

	ret = recv(sock, &buff[0], buff.size(), 0);
	ROUTN_LOG_INFO(g_logger) << "recv ret = " << ret << " errno = " << errno;

	if(ret <= 0) return ;

	buff.resize(ret);

	ROUTN_LOG_INFO(g_logger) << buff;
}

int main(int argc, char** argv){
	//test_sleep();
	//Routn::set_hook_enable(true);
	Routn::IOManager iom;
	iom.schedule(test_sock);
	return 0;
}

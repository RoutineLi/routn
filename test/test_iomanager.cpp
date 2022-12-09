/*************************************************************************
	> File Name: test_iomanager.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月01日 星期二 19时09分47秒
 ************************************************************************/

#include "../src/Routn.h"
#include "../src/IoManager.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>

Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

int sock = 0;

void test_fiber(){
	Routn::set_hook_enable(false);
	ROUTN_LOG_INFO(g_logger) << YELLOW << "test_fiber" << _NONE;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(sock, F_SETFL, O_NONBLOCK);

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8888);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

	if(!connect(sock, (struct sockaddr*)&addr, sizeof(addr))){
	}else if(errno == EINPROGRESS){
		ROUTN_LOG_INFO(g_logger) << "add event errno = " << errno << " "
			<< strerror(errno);
		Routn::IOManager::GetThis()->addEvent(sock, Routn::IOManager::WRITE, [](){
			ROUTN_LOG_INFO(g_logger) << YELLOW << "connected" << _NONE;});
	//	Routn::IOManager::GetThis()->addEvent(sock, Routn::IOManager::WRITE, [](){
	//		ROUTN_LOG_INFO(g_logger) << YELLOW << "connected" << _NONE;
	//	});
	}else{
		ROUTN_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
	}
	
	
}

void test1(){	
	Routn::IOManager iom;
	iom.schedule(&test_fiber);
}

void test_timer(){
	Routn::IOManager iom(2);
	iom.addTimer(1000, [](){
		ROUTN_LOG_INFO(g_logger) << YELLOW << "hello timer!" << _NONE;
	}, true);
}

int main(int argc, char** argv)
{
	test1();

	return 0;
}

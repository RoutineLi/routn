/*************************************************************************
	> File Name: test_daemon.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月27日 星期日 02时33分07秒
 ************************************************************************/

#include "../src/Daemon.h"
#include "../src/Routn.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();
static Routn::Timer::ptr timer;
int server_main(int argc, char** argv){
	Routn::IOManager iom(1);
	timer = iom.addTimer(1000, [](){
		ROUTN_LOG_INFO(g_logger) << Routn::ProcessInfoMgr::GetInstance()->toString();
		ROUTN_LOG_INFO(g_logger) << "on Timer!";
		static int i = 1;
		++i;
		if(i > 10){
			timer->cancel();
		}
	}, true);
	return 0;
}

int main(int argc, char** argv){
	return Routn::start_daemon(argc, argv, server_main, argc != 1);
}


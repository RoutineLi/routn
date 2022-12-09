/*************************************************************************
	> File Name: test_scheduler.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月31日 星期一 14时18分58秒
 ************************************************************************/

#include "../src/Routn.h"

Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

void test_fiber(){
    ROUTN_LOG_INFO(g_logger) << "test in fiber";
	static int s_count = 5;
	sleep(1);
	if(--s_count > 0){
		Routn::Schedular::GetThis()->schedule(&test_fiber);
	}
}

int main(int argc, char** argv){
	ROUTN_LOG_INFO(g_logger) << "main";
	Routn::Schedular sc(3, false, "test");
	sc.start();
	sleep(2);
    sc.schedule(&test_fiber);
	sc.stop();
	ROUTN_LOG_INFO(g_logger) << "over";
	return 0;
}


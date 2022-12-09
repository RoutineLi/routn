/*************************************************************************
	> File Name: test_fiber.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月30日 星期日 18时40分31秒
 ************************************************************************/

#include "../src/Routn.h"

void run_in_fiber(){
	ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << "run_in_fiber begin";
	Routn::Fiber::YieldToHold();
	ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << "run_in_fiber end";
	Routn::Fiber::YieldToHold();
}

void test_fiber(){
	Routn::Fiber::GetThis();
	ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << "main begin"; 
	Routn::Fiber::ptr fiber(new Routn::Fiber(run_in_fiber));
	fiber->swapIn();
	ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << "main after swapIn";
	fiber->swapIn();
	ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << "main after end";
	fiber->swapIn();
}

int main(int argc, char** argv) {
	Routn::Thread::SetName("main");
	
	std::vector<Routn::Thread::ptr> thrs;
	for(int i = 0; i < 3; ++i){
		thrs.push_back(Routn::Thread::ptr(new Routn::Thread(&test_fiber, "thread_" + std::to_string(i))));
	}
	for(auto i : thrs){
		i->join();
	}
	return 0;
}




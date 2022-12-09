/*************************************************************************
	> File Name: test_thread.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月29日 星期六 17时21分38秒
 ************************************************************************/

#include "../src/Routn.h"

Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();


int cnt = 0;


void fun1()
{
	ROUTN_LOG_INFO(g_logger) << "thread_name: " << Routn::Thread::GetName()
							 << " this name: " << Routn::Thread::GetThis()->GetName()
							 << " id: " << Routn::GetThreadId()
							 << " this.id: " << Routn::Thread::GetThis()->getId();
	for (int i = 0; i < 100; ++i)
	{
		//Routn::MutexType::Lock lock(s_mutex);
		++cnt;
	}
}

void fun2()
{
	while(1){
		ROUTN_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
	}
}

void fun3()
{
	while(1){
		ROUTN_LOG_INFO(g_logger) << "===========================================";
	}
}

int main(int argc, char **argv){
	ROUTN_LOG_INFO(g_logger) << "thread test begin";
	YAML::Node root = YAML::LoadFile("/home/rotun-li/routn/bin/conf/thread.yml");
	Routn::Config::LoadFromYaml(root);
	std::vector<Routn::Thread::ptr> thrs;
	for (int i = 0; i < 1; ++i)
	{
		Routn::Thread::ptr thr(new Routn::Thread(&fun2, "name_" + std::to_string(i * 2)));
		Routn::Thread::ptr thr2(new Routn::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
		thrs.push_back(thr);
		thrs.push_back(thr2);
	}

	for (size_t i = 0; i < thrs.size(); i++)
	{
		thrs[i]->join();
	}
	ROUTN_LOG_INFO(g_logger) << "thread test end";
	ROUTN_LOG_INFO(g_logger) << "cnt = " << cnt;
	return 0;
}

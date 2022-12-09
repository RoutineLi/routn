/*************************************************************************
	> File Name: test_address.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月06日 星期日 13时49分49秒
 ************************************************************************/

#include "../src/Address.h"
#include "../src/Routn.h"

Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

void test(){
	std::vector<Routn::Address::ptr> addrs;
	bool v = Routn::Address::Lookup(addrs, "www.baidu.com");
	if(!v){
		ROUTN_LOG_ERROR(g_logger) << "look up fail";
		return ;
	}
	for(size_t i = 0; i < addrs.size(); ++i){
		ROUTN_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
	}
}

void test2(){
	std::unordered_multimap<std::string, std::pair<Routn::Address::ptr, uint32_t>> res;
	bool v = Routn::Address::GetInterfaceAddress(res);
	if(!v){
		ROUTN_LOG_ERROR(g_logger) << "GetInterfaceAddress fail";
		return ;
	}
	for(auto &i : res){
		ROUTN_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - " << i.second.second;
	}
}

void test3(){
	auto addr = Routn::IPAddress::Create("www.baidu.com");
	if(addr){
		ROUTN_LOG_INFO(g_logger) << addr->toString();
	}
}

int main(int argc, char** argv){

	test2();
	return 0;
}


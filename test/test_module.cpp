/*************************************************************************
	> File Name: test_module.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月02日 星期一 18时48分48秒
 ************************************************************************/

#include "../src/Module.h"
#include "../src/Singleton.h"
#include <iostream>
#include "../src/Log.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

class A{
public:
	A(){
		//std::cout << "A::A" << std::endl;
	}
	~A(){
		//std::cout << "A::~A" << std::endl;
	}
};

class MyModule : public Routn::RockModule{
public:
	MyModule()
		:RockModule("hello", "1.0", ""){

	}

	bool onLoad() override{
		Routn::Singleton<A>::GetInstance();
		///std::cout << "--------onLoad-------" << std::endl;
		return true;
	}

	bool onUnload() override{
		Routn::Singleton<A>::GetInstance();
		///std::cout << "-------onUnload-------" << std::endl;
		return true;
	}

	bool onServerReady(){
		registerService("rock", "routn300.top", "blog");
	
		return true;
	}	

	bool handleRockRequest(Routn::RockRequest::ptr request
							, Routn::RockResponse::ptr response
							, Routn::RockStream::ptr stream){
		ROUTN_LOG_INFO(g_logger) << "handleRockRequest " << request->toString();
		response->setResult(0);
		response->setResStr("ok");
		response->setBody("hi iam routn300");

		return true;
	}

	bool handleRockNotify(Routn::RockNotify::ptr notify
							, Routn::RockStream::ptr stream){
		ROUTN_LOG_INFO(g_logger) << "handleRockNotify " << notify->toString();  
		return true;
	}
};

extern "C" {

Routn::Module* CreateModule() {
    Routn::Singleton<A>::GetInstance();
	ROUTN_LOG_INFO(g_logger) << "Create TestModule Success!";
    //std::cout << "=============CreateModule=================" << std::endl;
    return new MyModule;
}

void DestroyModule(Routn::Module* ptr) {
	
	ROUTN_LOG_INFO(g_logger) << "Destroy TestModule Success!";
    //std::cout << "=============DestoryModule=================" << std::endl;
    delete ptr;
}

}
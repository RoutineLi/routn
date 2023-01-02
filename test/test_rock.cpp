/*************************************************************************
	> File Name: test_rock.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月02日 星期一 19时09分20秒
 ************************************************************************/

#include<iostream>
#include "../src/Routn.h"
#include "../src/rpc/RockStream.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

Routn::RockConnection::ptr conn(new Routn::RockConnection);
void test(){
	conn->setAutoConnect(true);
	Routn::Address::ptr addr = Routn::Address::LookupAny("0.0.0.0:8060");
	if(!conn->connect(addr)){
		ROUTN_LOG_INFO(g_logger) << "connect " << *addr << " fail";
	}
	conn->start();

	Routn::IOManager::GetThis()->addTimer(1000, [](){
		Routn::RockRequest::ptr req(new Routn::RockRequest);
		static uint32_t s_sn = 0;
		req->setSn(++s_sn);
		req->setCmd(100);
		req->setBody("hello world sn = " + std::to_string(s_sn));

		auto rsp = conn->request(req, 300);
		if(rsp->response){
			ROUTN_LOG_INFO(g_logger) << rsp->response->toString();
		}else{
			ROUTN_LOG_INFO(g_logger) << "error result = " << rsp->result;
		}

	}, false);
}

int main(int argc, char** argv){
	Routn::IOManager iom(1);
	iom.schedule(test);
	return 0;
}


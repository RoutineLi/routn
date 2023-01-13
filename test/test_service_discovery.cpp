/*************************************************************************
	> File Name: test_service_discovery.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月12日 星期四 00时53分02秒
 ************************************************************************/

#include "../src/streams/ServiceDiscovery.h"
#include "../src/rpc/RockStream.h"
#include "../src/Worker.h"
#include "../src/Routn.h"

Routn::ZKServiceDiscovery::ptr zksd(new Routn::ZKServiceDiscovery("127.0.0.1:2181"));
Routn::RockSDLoadBalance::ptr rsdlb(new Routn::RockSDLoadBalance(zksd));

static Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

std::atomic<uint32_t> s_id;

void onTimer(){
	g_logger->setLevel(Routn::LogLevel::INFO);
	Routn::RockRequest::ptr req(new Routn::RockRequest);
	req->setSn(++s_id);
	req->setCmd(100);
	req->setBody("hello");

	auto rt = rsdlb->request("routn300.top", "blog", req, 1000);
	ROUTN_LOG_INFO(g_logger) << rt->toString();
}

// int add(int a, int b){
// 	return a + b;
// }

void run(){
	zksd->setSelfInfo("127.0.0.1:2222");
	zksd->setSelfData("aaaaa");

    std::unordered_map<std::string, std::unordered_map<std::string,std::string> > confs;
    confs["routn300.top"]["blog"] = "fair";
    rsdlb->start(confs);

	Routn::IOManager::GetThis()->addTimer(3000, onTimer, true);
}


int main(int argc, char** argv){
	Routn::IOManager iom(1);
	Routn::WorkerMgr::GetInstance()->init({
		{"service_io", {
			{"thread_num", "1"}
		}}
	});
	iom.schedule(run);
	iom.addTimer(1000, [](){
		std::cout << L_BLUE << rsdlb->statusString() << _NONE << std::endl;
	}, true);
	return 0;
}

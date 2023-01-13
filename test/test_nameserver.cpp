/*************************************************************************
	> File Name: test_nameserver.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月09日 星期一 02时47分47秒
 ************************************************************************/

#include "src/ns/NameServerClient.h"
#include "src/Routn.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

int type = 0;

void run(){
	g_logger->setLevel(Routn::LogLevel::INFO);
	auto addr = Routn::Address::LookupAny("0.0.0.0:8072");
	Routn::RockConnection::ptr conn(new Routn::RockConnection);	
	if(type == 0){
		for(int i = 0; i < 1; ++i){
			conn->connect(addr);
			Routn::IOManager::GetThis()->addTimer(3000, [conn, i](){
				Routn::RockRequest::ptr req(new Routn::RockRequest);
				req->setCmd((int)Routn::Ns::NSCommand::REGISTER);
				auto rinfo = std::make_shared<Routn::Ns::RegisterRequest>();
				auto info = rinfo->add_info();
				info->set_domain("0domain.com");
				info->add_cmds(rand() % 2 + 100);
				info->add_cmds(rand() % 2 + 200);
				info->mutable_node()->set_ip("127.0.0.1");
				info->mutable_node()->set_port(1000 + i);
				info->mutable_node()->set_weight(100);			
				req->setAsPB(*rinfo);
				std::string buff;
				rinfo->SerializeToString(&buff);
				std::cout << L_GREEN << buff << _NONE << std::endl;
				auto rt = conn->request(req, 300);
				ROUTN_LOG_INFO(g_logger) << "[result = "
					<< rt->result << " response = "
					<< (rt->response ? rt->response->toString() : "null")
					<< "]";
			}, false);
			conn->start();
		}
	
	}else{
		for(int i = 0; i < 1; ++i){
			Routn::Ns::NSClient::ptr nsclient(new Routn::Ns::NSClient);
			nsclient->init();
			nsclient->addQueryDomain("0domain.com");
			nsclient->connect(addr);
			nsclient->start();
			auto res = nsclient->query();
			auto pb = res->response->getAsPB<Routn::Ns::QueryResponse>();
			ROUTN_LOG_INFO(g_logger) << "NSClient start: " << Routn::PBToJsonString(*pb);

			if(i == 0){
				//  Routn::IOManager::GetThis()->addTimer(1000, [nsclient](){
				//  	auto domains = nsclient->getDomains();
				//  	domains->dump(std::cout, " ");
				//  }, true);
			}
		}

	// 	conn->setConnectCB([](Routn::AsyncSocketStream::ptr ss){
	// 		auto conn = std::dynamic_pointer_cast<Routn::RockConnection>(ss);
	// 		Routn::RockRequest::ptr req(new Routn::RockRequest);
	// 		req->setCmd((int)Routn::Ns::NSCommand::QUERY);
	// 		auto rinfo = std::make_shared<Routn::Ns::QueryRequest>();
	// 		rinfo->add_domains("0domain.com");
	// 		req->setAsPB(*rinfo);
	// 		auto rt = conn->request(req, 1000);
	// 		ROUTN_LOG_INFO(g_logger) << "[result="
	// 		<< rt->result << " response="
	// 		<< (rt->response ? rt->response->toString() : "null")
	// 		<< "]";
	// 		return true;
	// 	});

	// 	conn->setNotifyHandler([](Routn::RockNotify::ptr nty, Routn::RockStream::ptr stream){
	// 		auto nm = nty->getAsPB<Routn::Ns::NotifyMessage>();
	// 		if(!nm){
	// 			ROUTN_LOG_ERROR(g_logger) << "invalid notify msg!";
	// 			return true;
	// 		}
	// 		ROUTN_LOG_INFO(g_logger) << Routn::PBToJsonString(*nm);
	// 		return true;
	// 	});
	// 	conn->connect(addr);
	// 	conn->start();
	// }
	}
}

int main(int argc, char** argv){
	if(argc > 1){
		type = 1;
	}
	Routn::IOManager iom(5);
	iom.schedule(run);
	return 0;
}

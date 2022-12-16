/*************************************************************************
	> File Name: heart_module.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月28日 星期一 13时26分48秒
 ************************************************************************/

#include "heart_module.h"
#include "src/Log.h"
#include "src/Application.h"
#include "src/Config.h"
#include "src/Env.h"

static Routn::Logger::ptr root = ROUTN_LOG_ROOT();

HeartModule::HeartModule()
	: Module("heart", "1.0", ""){

}

bool HeartModule::onLoad() {
	return true;
}

bool HeartModule::onUnload() {
	return false;
}

bool HeartModule::onServerUp() {
	return true;
}

bool HeartModule::onServerReady() {
	std::vector<Routn::TcpServer::ptr> svrs;
	if(!Routn::Application::GetInstance()->getServer("http", svrs)){
		ROUTN_LOG_WARN(root) << "no http-server exist!";
		return false;
	}

	for(auto& i : svrs){
		Routn::Http::HttpServer::ptr sever = std::dynamic_pointer_cast<Routn::Http::HttpServer>(i);
		if(!i){
			continue;
		}

		auto slt = sever->getServletDispatch();
		slt->addServlet("/routn/love", [](Routn::Http::HttpRequest::ptr req, Routn::Http::HttpResponse::ptr rsp, Routn::Http::HttpSession::ptr session){
			ROUTN_LOG_INFO(root) << "req: " << L_GREEN << req->toString() << _NONE;
			std::ifstream fs;
			fs.open("/home/rotun-li/routn/bin/html/love.html");
			std::string buff;
			if(fs.is_open()){
				std::string tmp;
				while(getline(fs, tmp)){
					buff += tmp;
				}
			}
			fs.close();
			rsp->setBody(buff);
			return 0;
		});
	}
	return true;
}

extern "C"{
	Routn::Module* CreateModule(){
		Routn::Module* mod = new HeartModule;
		ROUTN_LOG_INFO(root) << "Create HeartModule Success!";
		return mod;
	}

	void DestroyModule(Routn::Module* module){
		ROUTN_LOG_INFO(root) << "Destroy HeartModule";
		delete module;
	}
	
}



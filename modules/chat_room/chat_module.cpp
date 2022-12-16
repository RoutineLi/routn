/*************************************************************************
	> File Name: chat_module.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月16日 星期五 19时39分57秒
 ************************************************************************/

#include "chat_module.h"
#include "chat_servlet.h"

#include "src/Application.h"
#include "src/Config.h"
#include "src/Log.h"
#include "src/Env.h"

static Routn::Logger::ptr root = ROUTN_LOG_ROOT();

namespace Chat{

	ChatModule::ChatModule()
	: Module("chat", "1.0", ""){

	}

	bool ChatModule::onLoad() {
		return true;
	}

	bool ChatModule::onUnload() {
		return false;
	}

	//static int32_t handle(Routn::Http::HttpRequest::ptr request
	//						, Routn::Http::HttpResponse::ptr response
	//						, Routn::Http::HttpSession::ptr session){
	//	ROUTN_LOG_INFO(root) << "handle";
	//	return 0;
    //}	

	bool ChatModule::onServerUp() {
		return true;
	}

	bool ChatModule::onServerReady() {
		std::vector<Routn::TcpServer::ptr> svrs;
		if(!Routn::Application::GetInstance()->getServer("http", svrs)){
			ROUTN_LOG_INFO(root) << "no httpserver alive";
			return false; 
		}
		for(auto& i : svrs){
			Routn::Http::HttpServer::ptr http_server =
				std::dynamic_pointer_cast<Routn::Http::HttpServer>(i);
			if(!i){
				continue;
			}
			Routn::Http::ResourceServlet::ptr slt(new Routn::Http::ResourceServlet(Routn::EnvMgr::GetInstance()->getCwd()));
			auto slt_dispatch = http_server->getServletDispatch();
			slt_dispatch->addGlobServlet("/html/*", slt);
		}
		return true;
	}

}

extern "C"{
	Routn::Module* CreateModule(){
		Routn::Module* mod = new Chat::ChatModule;
		ROUTN_LOG_INFO(root) << "Create ChatModule Success!";
		return mod;
	}

	void DestroyModule(Routn::Module* module){
		ROUTN_LOG_INFO(root) << "Destroy ChatModule";
		delete module;
	}
	
}


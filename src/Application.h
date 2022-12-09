/*************************************************************************
	> File Name: Application.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月27日 星期日 21时55分00秒
 ************************************************************************/

#ifndef _APPLICATION_H
#define _APPLICATION_H

#include <unordered_map>

#include "IoManager.h"
#include "Daemon.h"
#include "Log.h"
#include "Env.h"

#include "http/HttpServer.h"
#include "TcpServer.h"

namespace Routn{
	
	class Application{
	public:
		Application();

		static Application* GetInstance() { return s_instance;}

		bool getServer(const std::string& type, std::vector<TcpServer::ptr>& svrs);		
		bool init(int argc, char** argv);
		bool run();
	private:
		int _main(int argc, char** argv);
		int run_fiber();
	private:
		static Application* s_instance;
		std::unordered_map<std::string, std::vector<TcpServer::ptr> > _servers;
		IOManager::ptr _mainIOMgr;
		int _argc = 0;
		const char** _argv = nullptr;
	};

}

#endif

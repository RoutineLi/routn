/*************************************************************************
	> File Name: Application.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月27日 星期日 21时55分04秒
 ************************************************************************/

#include "Application.h"
#include "Worker.h"
#include "Module.h"
#include "Config.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

static Routn::ConfigVar<std::string>::ptr g_server_work_path = 
	Routn::Config::Lookup("system.work_path", std::string("/home/rotun-li/routn"), "server work path");
static Routn::ConfigVar<std::string>::ptr g_server_pid_file = 
	Routn::Config::Lookup("system.pid_file", std::string("routn.pid"), "server pid file");

//struct __INIT_WORK_PATH{
//	__INIT_WORK_PATH(){
//		g_server_work_path->addListener([](const std::string& ov, const std::string& nv){
//			g_server_work_path->setValue(nv);
//		});
//	}
//};

namespace Routn{

	static Routn::ConfigVar<std::vector<Routn::TcpServerConf>>::ptr g_tcp_server_conf =
		Routn::Config::Lookup("servers", std::vector<Routn::TcpServerConf>(), "server config");

	Application* Application::s_instance = nullptr;

	Application::Application(){
		s_instance = this;
	}

	bool Application::init(int argc, char** argv){
		_argc = argc, _argv = (const char**)argv;
		Routn::EnvMgr::GetInstance()->addHelpInfo("s", "start with the terminal");
		Routn::EnvMgr::GetInstance()->addHelpInfo("d", "run as daemon");
		Routn::EnvMgr::GetInstance()->addHelpInfo("c", "config path default: ./conf");
		Routn::EnvMgr::GetInstance()->addHelpInfo("h", "print help");
	
		if(!Routn::EnvMgr::GetInstance()->init(argc, argv)){
			Routn::EnvMgr::GetInstance()->printHelp();
			return false;
		}
		
		if(Routn::EnvMgr::GetInstance()->hasKey("p")){
			Routn::EnvMgr::GetInstance()->printHelp();
			return false;
		}

		std::string conf_path = Routn::EnvMgr::GetInstance()->getConfigPath();
		ROUTN_LOG_INFO(g_logger) << "loading conf path: " << conf_path;
		Routn::Config::LoadFromConfDir(conf_path);

		//static __INIT_WORK_PATH __initer__;	

		ModuleMgr::GetInstance()->init();
		std::vector<Module::ptr> modules;
		ModuleMgr::GetInstance()->listAll(modules);

		/**
		 * 业务模块解析参数
		 */
		for(auto i : modules){
			i->onBeforeArgsParse(argc, argv);
		}

		for(auto i : modules){
			i->onAfterArgsParse(argc, argv);
		}


		modules.clear();

		int run_type = 0;
		if(Routn::EnvMgr::GetInstance()->hasKey("s")){
			run_type = 1;
		}
		if(Routn::EnvMgr::GetInstance()->hasKey("d")){
			run_type = 2;
		}

		if(run_type == 0){
			Routn::EnvMgr::GetInstance()->printHelp();
			return false;
		}

		std::string pidfile = g_server_work_path->getValue()
								+ "/" + g_server_pid_file->getValue();
		if(Routn::FSUtil::IsRunningPidfile(pidfile)){
			ROUTN_LOG_ERROR(g_logger) << "server is running: " << pidfile;
			return false; 
		}

		if(!Routn::FSUtil::Mkdir(g_server_work_path->getValue())){
			ROUTN_LOG_FATAL(g_logger) << "create work path [" << g_server_work_path->getValue()
				<< " errno = " << errno << " reason: " << strerror(errno);
			return false;
		}

		return true;
	}	


	int Application::_main(int argc, char** argv){
		signal(SIGPIPE, SIG_IGN);
		auto pidfile = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();
		
		std::string conf_path = Routn::EnvMgr::GetInstance()->getConfigPath();
		ROUTN_LOG_INFO(g_logger) << "loading conf path: " << conf_path;
		Routn::Config::LoadFromConfDir(conf_path);
		
		std::ofstream ofs(pidfile);
		if(!ofs){
			ROUTN_LOG_ERROR(g_logger) << "open pidfile [" << pidfile << "] failed"; 
			return false;
		}
		ofs << getpid();

		_mainIOMgr.reset(new Routn::IOManager(1, true, "main"));
		_mainIOMgr->schedule(std::bind(&Application::run_fiber, this));
		_mainIOMgr->addTimer(2000, [conf_path](){
			Routn::Config::LoadFromConfDir(conf_path);
		}, true);
		_mainIOMgr->stop();
		return 0;
	}

	int Application::run_fiber(){
		Routn::WorkerMgr::GetInstance()->init();
		std::vector<Module::ptr> modules;
		ModuleMgr::GetInstance()->listAll(modules);
		for(auto &i : modules){
			if(!i->onLoad()){
				ROUTN_LOG_ERROR(g_logger) << "module: " << i->getName() << "load error";
				exit(EXIT_FAILURE);
			}
		}

		auto tcp_confs = g_tcp_server_conf->getValue();
		std::vector<TcpServer::ptr> svrs;
		for(auto& i : tcp_confs){
			ROUTN_LOG_INFO(g_logger) << i.toString();
			
			std::vector<Address::ptr> address;
			for(auto& a : i.address){
				size_t pos = a.find(":");
				if(pos == std::string::npos){
					address.push_back(UnixAddress::ptr(new UnixAddress(a)));
					continue;
				}
				int32_t port = atoi(a.substr(pos + 1).c_str());
				//127.0.0.1
				auto addr = Routn::IPAddress::Create(a.substr(0, pos).c_str(), port);
				if(addr){
					address.push_back(addr);
					continue;
				}
				std::vector<std::pair<Address::ptr, uint32_t> > res;
				if(Routn::Address::GetInterfaceAddress(res, a.substr(0, pos))){
					for(auto &x : res){
						auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
						if(ipaddr){
							ipaddr->setPort(atoi(a.substr(pos + 1).c_str()));
						}
						address.push_back(ipaddr);
					}	
					continue;
				}

				auto aaddr = Routn::Address::LookupAny(a);
				if(aaddr){
					address.push_back(aaddr);
					continue;
				}
				ROUTN_LOG_ERROR(g_logger) << "invalid address: " << a;
				exit(EXIT_FAILURE);
			}

			IOManager* accept_worker = Routn::IOManager::GetThis();
			IOManager* io_worker = Routn::IOManager::GetThis();
        	IOManager* process_worker = Routn::IOManager::GetThis();
			if(!i.io_worker.empty()){
				io_worker = Routn::WorkerMgr::GetInstance()->getAsIOManager(i.io_worker).get();
			}else{
				ROUTN_LOG_ERROR(g_logger) << "io_worker: " << i.io_worker
					<< " not exists";
				exit(EXIT_FAILURE);				
			}
			if(!i.accept_worker.empty()){
				accept_worker = Routn::WorkerMgr::GetInstance()->getAsIOManager(i.accept_worker).get();
			}else{
				ROUTN_LOG_ERROR(g_logger) << "accept_worker: " << i.accept_worker
					<< " not exists";
				exit(EXIT_FAILURE);
			}
		
			//if(!i.process_worker.empty()){
			// 	process_worker = Routn::WorkerMgr::GetInstance()->getAsIOManager(i.process_worker).get();
			//}else{
			// 	ROUTN_LOG_ERROR(g_logger) << "proc_worker: " << i.process_worker
			// 		<< " not exists";
			// 	exit(EXIT_FAILURE);
			//}
	
			TcpServer::ptr server;
			if(i.type == "http"){
				server.reset(new Routn::Http::HttpServer(i.keepalive, process_worker, io_worker, accept_worker));
				//server.reset(new Routn::Http::HttpServer(i.keepalive, Routn::IOManager::GetThis(), Routn::IOManager::GetThis(), accept_worker));
			}else{
            	ROUTN_LOG_ERROR(g_logger) << "invalid server type=" << i.type
                	<< LexicalCast<TcpServerConf, std::string>()(i);
            	exit(EXIT_FAILURE);
			}
			if(!i.name.empty()){
				server->setName(i.name);
			}
			std::vector<Address::ptr> fails;
			if(!server->bind(address, fails)){
				for(auto& x : fails){
					ROUTN_LOG_ERROR(g_logger) << "bind address fail: " << *x;
				}
				exit(EXIT_FAILURE);
			}
			server->setConf(i);
			_servers[i.type].push_back(server);
			svrs.push_back(server);

			for(auto& i : modules){
				i->onServerReady();
			}

			for(auto &i : svrs){
				i->start();
			}

			for(auto& i : modules){
				i->onServerUp();
			}
		}
		return 0;
	}
	

	bool Application::getServer(const std::string& type, std::vector<TcpServer::ptr>& svrs) {
		auto it = _servers.find(type);
		if(it == _servers.end()) {
			return false;
		}
		svrs = it->second;
		return true;
	}

	bool Application::run(){
		bool status = Routn::EnvMgr::GetInstance()->hasKey("d");
		return start_daemon(_argc, const_cast<char** >(_argv), std::bind(&Application::_main, this
							, std::placeholders::_1
							, std::placeholders::_2), status);
	}

}


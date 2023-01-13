/*************************************************************************
	> File Name: Module.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月28日 星期一 09时37分15秒
 ************************************************************************/

#include "Module.h"
#include "Config.h"
#include "Env.h"
#include "Log.h"
#include "Util.h"
#include "Library.h"
#include "Application.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

static Routn::ConfigVar<std::string>::ptr g_module_path = 
	Routn::Config::Lookup("module.path", std::string("module"), "module path");

namespace Routn{

	Module::Module(const std::string& name
		, const std::string& version
		, const std::string& filename
		, uint32_t type)
			: _name(name)
			, _version(version)
			, _filename(filename)
			, _id(name + "/" + version)
			, _type(type){

	}

	void Module::onBeforeArgsParse(int argc, char** argv){
	}

	void Module::onAfterArgsParse(int argc, char** argv){

	}


	bool Module::onLoad(){
		return true;
	}

	bool Module::onUnload(){
		return false;
	}


	bool Module::onConnect(Routn::Stream::ptr stream){
		return true;
	}

	bool Module::onDisconnect(Routn::Stream::ptr stream){
		return true;
	}


	bool Module::onServerReady(){
		return true;
	}

	bool Module::onServerUp(){
		return true;
	}

	bool RockModule::handleRequest(Message::ptr req
						,Message::ptr rsp
						,Stream::ptr stream){
		auto rock_req = std::dynamic_pointer_cast<RockRequest>(req);
		auto rock_rsp = std::dynamic_pointer_cast<RockResponse>(rsp);
		auto rock_stream = std::dynamic_pointer_cast<RockStream>(stream);
    	return handleRockRequest(rock_req, rock_rsp, rock_stream);
	}
	
	bool RockModule::handleNotify(Message::ptr notify
							,Stream::ptr stream){
		auto rock_nty = std::dynamic_pointer_cast<RockNotify>(notify);
		auto rock_stream = std::dynamic_pointer_cast<RockStream>(stream);
		return handleRockNotify(rock_nty, rock_stream);
	}

	void RockModule::registerService(const std::string& server_type,
            							const std::string& domain, const std::string& service){
		auto sd = Application::GetInstance()->getServiceDiscovery();
		if(!sd){
			return;
		}
		std::vector<TcpServer::ptr> svrs;
		if(!Application::GetInstance()->getServer(server_type, svrs)){
			return ;
		}
		for(auto& i : svrs){
			auto socks = i->getSocks();
			for(auto& s : socks){
				auto addr = std::dynamic_pointer_cast<IPv4Address>(s->getLocalAddress());
				if(!addr){
					continue;
				}
				auto str = addr->toString();
				if(str.find("127.0.0.1") == 0){
					continue;
				}
				std::string ip_and_port;
				if(str.find("0.0.0.0") == 0){
					ip_and_port = Routn::GetIPv4() + ":" + std::to_string(addr->getPort());
				}else{
					ip_and_port = addr->toString();
				}
				sd->registerServer(domain, service, ip_and_port, server_type);
			}
		}
	}

	RockModule::RockModule(const std::string& name
							, const std::string& version
							, const std::string& filename)
		: Module(name, version, filename, ROCK){

	}

	ModuleManager::ModuleManager(){

	}

	void ModuleManager::add(Module::ptr m){
		del(m->getId());
		RWMutexType::WriteLock lock(_mutex);
		_modules[m->getId()] = m;
		_type2Modules[m->getType()][m->getId()] = m;
	}	

	void ModuleManager::del(const std::string& name){
		Module::ptr module;
		RWMutexType::WriteLock lock(_mutex);
		auto it = _modules.find(name);
		if(it == _modules.end()){
			return ;
		}
		module = it->second;
		_modules.erase(it);
		_type2Modules[module->getType()].erase(module->getId());
		if(_type2Modules[module->getType()].empty()){
			_type2Modules.erase(module->getType());
		}
		lock.unlock();
		module->onUnload();
	}

	void ModuleManager::delAll(){
		RWMutexType::ReadLock lock(_mutex);
		auto tmp = _modules;
		lock.unlock();
		
		for(auto& i : tmp){
			del(i.first);
		}
	}

	void ModuleManager::init(){
		auto path = EnvMgr::GetInstance()->getAbsolutePath(g_module_path->getValue());
		ROUTN_LOG_DEBUG(g_logger) << "module path: " << path;

		std::vector<std::string> files;
		FSUtil::ListAllFile(files, path, ".so");

		std::sort(files.begin(), files.end());
		for(auto& i : files){
			initModule(i);
		}
	}

	Module::ptr ModuleManager::get(const std::string& name){
		RWMutexType::ReadLock lock(_mutex);
		auto it = _modules.find(name);
		return it == _modules.end() ? nullptr : it->second;
	}

	void ModuleManager::onConnect(Stream::ptr stream){
		std::vector<Module::ptr> ms;
		listAll(ms);

		for(auto& m : ms){
			m->onConnect(stream);
		}
	}

	void ModuleManager::onDisconnect(Stream::ptr stream){
		std::vector<Module::ptr> ms;
		listAll(ms);

		for(auto& m : ms){
			m->onDisconnect(stream);
		}
	}

	void ModuleManager::listAll(std::vector<Module::ptr>& ms){
		RWMutexType::ReadLock lock(_mutex);
		for(auto &i : _modules){
			ms.push_back(i.second);
		}
	}

	void ModuleManager::listByType(uint32_t type, std::vector<Module::ptr>& ms){
		RWMutexType::ReadLock lock(_mutex);
		auto it = _type2Modules.find(type);
		if(it == _type2Modules.end()){
			return ;
		}
		for(auto& i : it->second){
			ms.push_back(i.second);
		}
	}

	void ModuleManager::foreach(uint32_t type, std::function<void(Module::ptr)> cb){
		std::vector<Module::ptr> ms;
		listByType(type, ms);
		for(auto &i : ms){
			cb(i);
		}
	}


	void ModuleManager::initModule(const std::string& path){
		Module::ptr m = Library::GetModule(path);
		if(m){
			add(m);
		}
	}

}


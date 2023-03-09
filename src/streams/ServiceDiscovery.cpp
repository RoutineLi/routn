/*************************************************************************
	> File Name: ServiceDiscovery.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月10日 星期二 23时08分42秒
 ************************************************************************/

#include "ServiceDiscovery.h"
#include "../Config.h"
#include "../Config.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace Routn{
	
	ServiceItemInfo::ptr ServiceItemInfo::Create(const std::string& ip_and_port, const std::string& data){
		auto pos = ip_and_port.find(':');
		if(pos == std::string::npos){
			return nullptr;
		}
		auto ip = ip_and_port.substr(0, pos);
		int16_t port = atoi(ip_and_port.substr(pos + 1).c_str());
		in_addr_t ip_addr = inet_addr(ip.c_str());
		if(ip_addr == 0){
			return nullptr;
		}

		ServiceItemInfo::ptr rt(new ServiceItemInfo);
		rt->_id = ((uint64_t)ip_addr << 32) | port;
		rt->_ip = ip;
		rt->_port = port;
		rt->_data = data;

		std::vector<std::string> parts = split(data, '&');
		for(auto& i : parts){
			auto pos = i.find('=');
			if(pos == std::string::npos) 
				continue;
			rt->_datas[i.substr(0, pos)] = i.substr(pos + 1);
		}
		rt->_updateTime = rt->getDataAs<uint64_t>("update_time");
		rt->_type = rt->getData("type");
		return rt;
	}

	std::string ServiceItemInfo::toString() const{
		std::stringstream ss;
		ss << "[ServiceItemInfo id = " << _id
		   << " ip = " << _ip
		   << " port = " << _port
		   << " data = " << _data
		   << "]";
		return ss.str();
	}

	std::string ServiceItemInfo::getData(const std::string& key, const std::string& def) const{
		auto it = _datas.find(key);
		return it == _datas.end() ? def : it->second;
	}


	void IServiceDiscovery::registerServer(const std::string& domain, const std::string& service,
						const std::string& ip_and_port, const std::string& data){
		RW_Mutex::WriteLock lock(_mutex);
		_registerInfos[domain][service][ip_and_port] = data;
	}

	void IServiceDiscovery::queryServer(const std::string& domain, const std::string& service){
		RW_Mutex::WriteLock lock(_mutex);
		_queryInfos[domain].emplace(service);
	}

	void IServiceDiscovery::listServer(std::unordered_map<std::string, std::unordered_map<std::string
						, std::unordered_map<uint64_t, ServiceItemInfo::ptr>> >& infos){
		RW_Mutex::ReadLock lock(_mutex);
		infos = _datas;
	}
	void IServiceDiscovery::listRegisterServer(std::unordered_map<std::string, std::unordered_map<std::string
						,std::unordered_map<std::string, std::string> > >& infos){
		RW_Mutex::ReadLock lock(_mutex);
		infos = _registerInfos;
	}

	void IServiceDiscovery::listQueryServer(std::unordered_map<std::string, std::unordered_set<std::string>> & infos){
		RW_Mutex::ReadLock lock(_mutex);
		infos = _queryInfos;
	}

	
	void IServiceDiscovery::addServiceCallback(service_callback v){
		RW_Mutex::WriteLock lock(_mutex);
		_cbs.push_back(v);
	}

	void IServiceDiscovery::setQueryServer(const std::unordered_map<std::string, std::unordered_set<std::string> >& v){
		RW_Mutex::WriteLock lock(_mutex);
		//TODO
		for(auto& i : v){
			auto& m = _queryInfos[i.first];
			for(auto& x : i.second){
				m.emplace(x);
			}
		}
	}

	void IServiceDiscovery::addParam(const std::string& key, const std::string& val){
		RW_Mutex::WriteLock lock(_mutex);
		_params[key] = val;
	}

	std::string IServiceDiscovery::getParam(const std::string& key, const std::string& def){
		RW_Mutex::ReadLock lock(_mutex);
		auto it = _params.find(key);
		return it == _params.end() ? def : it->second;
	}

	

	std::string IServiceDiscovery::toString(){
	    std::stringstream ss;
		RW_Mutex::ReadLock lock(_mutex);
		ss << "[Params]" << std::endl;
		for(auto& i : _params) {
			ss << "\t" << i.first << ":" << i.second << std::endl;
		}
		ss << std::endl;
		ss << "[register_infos]" << std::endl;
		for(auto& i : _registerInfos) {
			ss << "\t" << i.first << ":" << std::endl;
			for(auto& n : i.second) {
				ss << "\t\t" << n.first << ":" << std::endl;
				for(auto& x : n.second) {
					ss << "\t\t\t" << x.first << ":" << x.second << std::endl;
				}
			}
		}
		ss << std::endl;
		ss << "[query_infos]" << std::endl;
		for(auto& i : _queryInfos) {
			ss << "\t" << i.first << ":" << std::endl;
			for(auto& n : i.second) {
				ss << "\t\t" << n << std::endl;
			}
		}
		ss << std::endl;
		ss << "[services]" << std::endl;
		for(auto& i : _datas) {
			ss << "\t" << i.first << ":" << std::endl;
			for(auto& n : i.second) {
				ss << "\t\t" << n.first << ":" << std::endl;
				for(auto& x : n.second) {
					ss << "\t\t\t" << x.first << ": " << x.second->toString() << std::endl;
				}
			}
		}
		lock.unlock();
		return ss.str();	
	}



	ZKServiceDiscovery::ZKServiceDiscovery(const std::string& hosts)
		: _hosts(hosts){

	}

	void ZKServiceDiscovery::start(){
		if(_client){
			return ;
		}
		auto self = shared_from_this();
		_client.reset(new Routn::ZKClient);
		bool b = _client->init(_hosts, 6000, std::bind(&ZKServiceDiscovery::onWatch
			, self, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		if(!b){
			ROUTN_LOG_ERROR(g_logger) << "ZKClient init fail, hosts = " << _hosts;
		}
		_timer = IOManager::GetThis()->addTimer(60 * 1000, [self, this](){
			_isOnTimer = true;
			onZKConnect("", _client);
			_isOnTimer = false;
		}, true);
	}

	void ZKServiceDiscovery::stop(){
		if(_client){
			_client->close();
			_client = nullptr;
		}
		if(_timer){
			_timer->cancel();
			_timer = nullptr;
		}
	}


	bool ZKServiceDiscovery::doRegister(){
		/// @brief TODO
		/// @return 
		return true;
	}

	bool ZKServiceDiscovery::doQuery(){
		/// @brief TODO
		/// @return 
		return true;
	}


	static std::string GetProvidersPath(const std::string& domain, const std::string& service){
		return "/routn/" + domain + "/" + service + "/providers";
	}

	static std::string GetConsumersPath(const std::string& domain, const std::string& service){
		return "/routn/" + domain + "/" + service + "/consumers";
	}

	static std::string GetDomainPath(const std::string& domain){
		return "/routn/" + domain;
	}

	static bool ParseDomainService(const std::string& path, std::string& domain, std::string& service){
		auto v = Routn::split(path, '/');
		if(v.size() != 5){
			return false;
		}
		domain = v[2];
		service = v[3];
		return true;
	}

	void ZKServiceDiscovery::onWatch(int type, int stat, const std::string& path, ZKClient::ptr client){
		if(stat == ZKClient::StateType::CONNECTED){
			if(type == ZKClient::EventType::SESSION){
				return onZKConnect(path, client);
			}else if(type == ZKClient::EventType::CHILD){
				return onZKChild(path, client);
			}else if(type == ZKClient::EventType::CHANGED){
				return onZKChanged(path, client);
			}else if(type == ZKClient::EventType::DELETED){
				return onZKDeleted(path, client);
			}
		}else if(stat == ZKClient::StateType::EXPIRED_SESSION){
			if(type == ZKClient::EventType::SESSION){
				return onZKExpiredSession(path, client);
			}
		}
		ROUTN_LOG_ERROR(g_logger) << "onWatch hosts = " << _hosts
			<< " type = " << type << " stat = " << stat
			<< " path = " << path << " client = " << client << " is invalid";
	}

	void ZKServiceDiscovery::onZKConnect(const std::string& path, ZKClient::ptr client){
		RW_Mutex::ReadLock lock(_mutex);
		auto rinfo = _registerInfos;
		auto qinfo = _queryInfos;
		lock.unlock();
		bool ok = true;
		
		for(auto& i : rinfo){
			for(auto& x : i.second){
				for(auto& v : x.second){
									   //domain, service, ip&port, data
					ok &= registerInfo(i.first, x.first, v.first, v.second);
				}
			}
		}

		if(!ok){
			ROUTN_LOG_ERROR(g_logger) << "onZKConnect register fail";
		}
		ok = true;
		for(auto& i : qinfo){
			for(auto& x : i.second){
				ok &= queryInfo(i.first, x);
			}
		}
		if(!ok){
			ROUTN_LOG_ERROR(g_logger) << "onZKConnect query fail";
		}

		ok = true;
		for(auto& i : qinfo){
			for(auto& x : i.second){
				ok &= queryData(i.first, x);
			}
		}
		if(!ok){
			ROUTN_LOG_ERROR(g_logger) << "onZKConnect queryData fail";
		}
	}

	void ZKServiceDiscovery::onZKChild(const std::string& path, ZKClient::ptr client){
		getChildren(path);
	}

	void ZKServiceDiscovery::onZKChanged(const std::string& path, ZKClient::ptr client){
		ROUTN_LOG_INFO(g_logger) << "onZKChanged path = " << path;
	}

	void ZKServiceDiscovery::onZKDeleted(const std::string& path, ZKClient::ptr client){
		ROUTN_LOG_INFO(g_logger) << "onZKDeleted path = " << path;
	}

	void ZKServiceDiscovery::onZKExpiredSession(const std::string& path, ZKClient::ptr client){
		ROUTN_LOG_INFO(g_logger) << "onZKExpiredSession path = " << path;
		client->reconnect();
	}

	bool ZKServiceDiscovery::registerInfo(const std::string& domain, const std::string& service, 
						const std::string& ip_and_port, const std::string& data){
		std::string path = GetProvidersPath(domain, service);
		bool v = existsOrCreate(path);
		if(!v){
			ROUTN_LOG_ERROR(g_logger) << "create path = " << path << " fail";
			return false;
		}

		std::string new_val(1024, 0);
		int32_t rt = _client->create(path + "/" + ip_and_port, data, new_val
									, &ZOO_OPEN_ACL_UNSAFE, ZKClient::FlagsType::EPHEMERAL);
		if(rt == ZOK){
			return true;
		}
		if(!_isOnTimer){
			ROUTN_LOG_ERROR(g_logger) << "create path = " << (path + "/" + ip_and_port) << "fail, error:"
				<< zerror(rt);
		}
		return rt == ZNODEEXISTS;

	}

	bool ZKServiceDiscovery::queryInfo(const std::string& domain, const std::string& service){
		if(service != "all"){
			std::string path = GetConsumersPath(domain, service);
			bool v = existsOrCreate(path);
			if(!v){
				ROUTN_LOG_ERROR(g_logger) << "create path = " << path << " fail";
				return false;
			}
			if(_selfInfo.empty()){
				ROUTN_LOG_ERROR(g_logger) << "queryInfo selfInfo is null";
				return false;
			}

			std::string new_val(1024, 0);
			int32_t rt = _client->create(path + "/" + _selfInfo, _selfData, new_val
										, &ZOO_OPEN_ACL_UNSAFE, ZKClient::FlagsType::EPHEMERAL);
			if(rt == ZOK){
				return true;
			}
			if(!_isOnTimer){
				ROUTN_LOG_ERROR(g_logger) << "create path = " << (path + "/" + _selfInfo) << "fail"
					<< zerror(rt) << "( " << rt << " )";
			}
			return rt == ZNODEEXISTS;
		}else{
			std::vector<std::string> children;
			_client->getChildren(GetDomainPath(domain), children, false);
			bool rt = true;
			for(auto& i : children){
				rt &= queryInfo(domain, i);
			}
			return rt;
		}
	}

	bool ZKServiceDiscovery::queryData(const std::string& domain, const std::string& service){
		if(service != "all"){
			std::string path = GetProvidersPath(domain, service);
			return getChildren(path);
		}else{
			std::vector<std::string> children;
			_client->getChildren(GetDomainPath(domain), children, false);
			bool rt = true;
			for(auto& i : children){
				rt &= queryData(domain, i);
			}
			return rt;
		}
	}


	bool ZKServiceDiscovery::existsOrCreate(const std::string& path){
		int32_t v = _client->exists(path, false);
		if(v == ZOK){
			return true;
		}else{
			auto pos = path.find_last_of('/');
			if(pos == std::string::npos){
				ROUTN_LOG_ERROR(g_logger) << "existsOrCreate() invalid path = " << path;
				return false;
			}
			if(pos == 0 || existsOrCreate(path.substr(0, pos))){
				std::string new_val(1024, 0);
				v = _client->create(path, "", new_val);
				if(v != ZOK){
					ROUTN_LOG_ERROR(g_logger) << "create path = " << path << " error:"
						<< zerror(v) << " (" << v << " )";
					return false;
				}
				return true;
			}
		}
		return false;
	}

	bool ZKServiceDiscovery::getChildren(const std::string& path){
		std::string domain
					, service;
		if(!ParseDomainService(path, domain, service)){
			ROUTN_LOG_ERROR(g_logger) << "get_children path = " << path
				<< " invalid path";
			return false;
		}
		{
			RW_Mutex::ReadLock lock(_mutex);
			auto it = _queryInfos.find(domain);
			if(it == _queryInfos.end()){
				ROUTN_LOG_ERROR(g_logger) << "get_children path = " << path
					<< " domain = " << domain << " not exists";
				return false;
			}
			if(it->second.count(service) == 0 && it->second.count("all") == 0){
				ROUTN_LOG_ERROR(g_logger) << "get_children path = " << path
					<< " service = " << service << " not exists "
					<< Routn::Join(it->second.begin(), it->second.end(), ",");
				return false;
			}

		}

		std::vector<std::string> vals;
		int32_t v = _client->getChildren(path, vals, true);
		if(v != ZOK){
			ROUTN_LOG_ERROR(g_logger) << "get_children path = " << path << " fail, error:"
				<< zerror(v) << " (" << v << " )";
			return false;
		}
		std::unordered_map<uint64_t, ServiceItemInfo::ptr> infos;
		for(auto& i : vals){
			auto info = ServiceItemInfo::Create(i, "");
			if(!info){
				continue;
			}
			infos[info->getId()] = info;
			ROUTN_LOG_INFO(g_logger) << "domain = " << domain 
				<< " service = " << service << " info = " << info->toString();
		}

		auto new_vals = infos;
		RW_Mutex::WriteLock lock(_mutex);
		_datas[domain][service].swap(infos);
		auto cbs = _cbs;
		lock.unlock();

		for(auto& cb : cbs){
			cb(domain, service, infos, new_vals);
		}
		return true;
	}

}


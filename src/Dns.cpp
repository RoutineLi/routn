/*************************************************************************
	> File Name: Dns.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月15日 星期四 23时14分30秒
 ************************************************************************/

#include "Dns.h"
#include "Log.h"
#include "Config.h"
#include "IoManager.h"
#include "http/HttpConnection.h"
#include <cstdlib>
#include <cstdio>


static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn{

/**
 * @brief DNS load-config && init
 * 
 */

	struct DnsDefine{
		std::string domain;
		int type;
		int pool_size = 0;
		std::string check_path;
		std::set<std::string> addrs;

		bool operator==(const DnsDefine& b) const {
			return domain == b.domain
				&& type == b.type
				&& addrs == b.addrs
				&& check_path == b.check_path;
		}

		bool operator<(const DnsDefine& b) const {
			if(domain != b.domain){
				return domain < b.domain;
			}
			return false;
		}
	};

	template<>
	class LexicalCast<std::string, DnsDefine> {
	public:
		DnsDefine operator()(const std::string& v) {
			YAML::Node n = YAML::Load(v);
			DnsDefine dd;
			if(!n["domain"].IsDefined()) {
				return DnsDefine();
			}
			dd.domain = n["domain"].as<std::string>();
			if(!n["type"].IsDefined()) {
				return DnsDefine();
			}
			dd.type = n["type"].as<int>();

			if(n["check_path"].IsDefined()) {
				dd.check_path = n["check_path"].as<std::string>();
			}

			if(n["addrs"].IsDefined()) {
				for(size_t x = 0; x < n["addrs"].size(); ++x) {
					dd.addrs.insert(n["addrs"][x].as<std::string>());
				}
			}

			if(n["pool"].IsDefined()) {
				dd.pool_size = n["pool"].as<int>();
			}
			return dd;
		}
	};

	template<>
	class LexicalCast<DnsDefine, std::string> {
	public:
		std::string operator()(const DnsDefine& i) {
			YAML::Node n;
			n["domain"] = i.domain;
			n["type"] = i.type;
			n["pool"] = i.pool_size;
			if(!i.check_path.empty()) {
				n["check_path"] = i.check_path;
			}
			for(auto& x : i.addrs) {
				n["addrs"].push_back(x);
			}
			std::stringstream ss;
			ss << n;
			return ss.str();
		}
	};

	static Routn::ConfigVar<std::set<DnsDefine> >::ptr g_dns_defines = 
		Routn::Config::Lookup("dns.config", std::set<DnsDefine>(), "dns config");

	struct DnsIniter{
		DnsIniter(){
			g_dns_defines->addListener([](const std::set<DnsDefine>& old_value, const std::set<DnsDefine>& new_value){
				for(auto &n : new_value){
					if(n.type == Dns::TYPE_DOMAIN){
						Dns::ptr dns = std::make_shared<Dns>(n.domain, n.type, n.pool_size);
						dns->setCheckPath(n.check_path);
						dns->refresh();
						DnsMgr::GetInstance()->add(dns);
					}else if(n.type == Dns::TYPE_ADDRESS){
						Dns::ptr dns = std::make_shared<Dns>(n.domain, n.type, n.pool_size);
						dns->setCheckPath(n.check_path);
						dns->set(n.addrs);
						dns->refresh();
						DnsMgr::GetInstance()->add(dns);
					}else{
						ROUTN_LOG_INFO(g_logger) << "invalid type = " << n.type
							<< " domain = " << n.domain;
					}
				}
			});
		}
	};

	static DnsIniter __dns_init__;


/**
 * @brief source code with Dns && DnsMgr
 * 
 */

	Dns::Dns(const std::string& domain, int type, int32_t pool_size)
		: _domain(domain)
		, _type(type)
		, _idx(0)
		, _poolsize(pool_size) {
	}

	void Dns::set(const std::set<std::string>& addrs) {
		{
			RWMutexType::WriteLock lock(_mutex);
			_addrs = addrs;
		}
		init();
	}

	void Dns::init(){
		if(_type != TYPE_ADDRESS){
			ROUTN_LOG_ERROR(g_logger) << _domain << "invalid type: " << _type;
			return ;
		}

		RWMutexType::ReadLock lock2(_mutex);
		auto addrs = _addrs;
		lock2.unlock();

		std::vector<Address::ptr> result;
		for(auto &i : addrs){
			if(!Address::Lookup(result, i, Routn::Socket::IPV4, Routn::Socket::TCP)){
				ROUTN_LOG_ERROR(g_logger) << _domain << "invalid address: " << i;
			}
		}
		initAddress(result);
	}

	Routn::Address::ptr Dns::get(uint32_t seed){
		if(seed == (uint32_t)-1){
			seed = Routn::Atomic::addFetch(_idx);
		}
		RWMutexType::ReadLock lock(_mutex);
		for(size_t i = 0; i < _address.size(); ++i){
			auto info = _address[seed % _address.size()];
			if(info->valid){
				return info->addr;
			}
			seed = Routn::Atomic::addFetch(_idx);
		}
		return nullptr;
	}

	Routn::Socket::ptr Dns::getSock(uint32_t seed){
		if(seed == (uint32_t)-1){
			seed = Routn::Atomic::addFetch(_idx);
		}
		RWMutexType::ReadLock lock(_mutex);
		for(size_t i = 0; i < _address.size(); ++i){
			auto info = _address[(seed + i) % _address.size()];
			auto sock = info->getSock();
			if(sock){
				return sock;
			}
		}
		return nullptr;	
	}

	std::string Dns::toString() {
		std::stringstream ss;
		ss << "[Dns domain=" << _domain
		<< " type=" << _type
		<< " idx=" << _idx;
		if(!_checkPath.empty()) {
			ss << " check_path=" << _checkPath;
		}
		RWMutexType::ReadLock lock(_mutex);
		ss << " addrs.size=" << _address.size() << " addrs=[";
		for(size_t i = 0; i < _address.size(); ++i) {
			if(i) {
				ss << ",";
			}
			ss << _address[i]->toString();
		}
		lock.unlock();
		ss << "]";
		return ss.str();
	}

	bool Dns::AddressItem::isValid(){
		return valid;
	}

	std::string Dns::AddressItem::toString(){
		std::stringstream ss;
		ss << *addr << ":" << valid;
		if(pool_size > 0) {
			Routn::SpinLock::Lock lock(_mutex);
			ss << ":" << socks.size();
		}
		return ss.str();		
	}

	bool Dns::AddressItem::checkValid(uint32_t timeout_ms){
		if(pool_size > 0){
			std::vector<Socket*> tmp;
			Routn::SpinLock::Lock lock(_mutex);
			for(auto it = socks.begin(); it != socks.end();){
				if((*it)->isConnected()){
					return true;
				}else{
					tmp.push_back(*it);
					socks.erase(it++);
				}
			}
			lock.unlock();
			for(auto& i : tmp){
				delete i;
			}
		}

		Routn::Socket* sock = new Routn::Socket(addr->getFamily(), Socket::TCP);
		valid = sock->connect(addr, timeout_ms);

		if(valid){
			if(!check_path.empty()){
				Routn::Http::HttpRequest::ptr req = std::make_shared<Routn::Http::HttpRequest>();
				req->setPath(check_path);
				req->setHeader("host", addr->toString());
				Routn::Socket::ptr sock_ptr(sock, Routn::nop<Socket>);
				auto ret = Routn::Http::HttpConnection::DoRequest_base(req, sock_ptr, timeout_ms);
				if(!ret->response || (int)ret->response->getStatus() != 200) {
					valid = false;
					ROUTN_LOG_ERROR(g_logger) << "health_check fail result=" << ret->result
						<< " rsp.status=" << (ret->response ? (int)ret->response->getStatus() : -1)
						<< " check_path=" << check_path
						<< " addr=" << addr->toString();
            	}
			}
			if(valid && pool_size > 0){
				Routn::SpinLock::Lock lock(_mutex);
				socks.push_back(sock);
			}else{
				delete sock;
			}
		}else{
			delete sock;
		}
		return valid;
	}

	Dns::AddressItem::~AddressItem(){
		for(auto &i : socks){
			delete i;
		}
	}

	void Dns::AddressItem::push(Socket* sock){
		if(sock->checkConnected()){
			Routn::SpinLock::Lock lock(_mutex);
			socks.push_back(sock);
		}else{
			delete sock;
		}
	}

	static void ReleaseSock(Socket* sock, Dns::AddressItem::ptr ai){
		ai->push(sock);
	}

	Socket::ptr Dns::AddressItem::pop(){
		if(pool_size == 0){
			return nullptr;
		}
		Routn::SpinLock::Lock lock(_mutex);
		if(socks.empty()){
			return nullptr;
		}
		auto ret = socks.front();
		socks.pop_front();
		lock.unlock();

		Socket::ptr v(ret, std::bind(ReleaseSock, std::placeholders::_1, shared_from_this()));
		return v;
	}

	Socket::ptr Dns::AddressItem::getSock(){
		if(pool_size > 0){
			do{
				auto sock = pop();
				if(!sock){
					break;
				}
				if(sock->checkConnected()){
					return sock;
				}
			}while(true);
		}else if(valid){
			Routn::Socket* sock = new Routn::Socket(addr->getFamily(), Socket::TCP, 0);
			if(sock->connect(addr, 20)){
				if(pool_size > 0){
					Socket::ptr v(sock, std::bind(ReleaseSock, std::placeholders::_1, shared_from_this()));
				}else{
					return Routn::Socket::ptr(sock);
				}
			}else{
				delete sock;
			}
		}
		return nullptr;
	}

	void Dns::initAddress(const std::vector<Address::ptr>& result){
		std::map<std::string, AddressItem::ptr> old_addr;
		{
			RWMutexType::ReadLock lock(_mutex);
			auto tmp = _address;
			lock.unlock();

			for(auto& i : tmp){
				old_addr[i->addr->toString()] = i;
			}
		}

		std::vector<AddressItem::ptr> addr;
		addr.resize(result.size());
		std::map<std::string, AddressItem::ptr> m;
		for(size_t i = 0; i < result.size(); ++i){
			auto it = old_addr.find(result[i]->toString());
			if(it != old_addr.end()){
				it->second->checkValid(50);
				addr[i] = it->second;
				continue;
			}
			auto info = std::make_shared<AddressItem>();
			info->addr = result[i];
			info->pool_size = _poolsize;
			info->check_path = _checkPath;
			info->checkValid(50);
			addr[i] = info;
		}

		RWMutexType::WriteLock lock(_mutex);
		_address.swap(addr);
	}

	void Dns::refresh(){
		if(_type == TYPE_DOMAIN){
			std::vector<Address::ptr> result;
			if(!Routn::Address::Lookup(result, _domain, Socket::IPV4, Socket::TCP)){
				ROUTN_LOG_ERROR(g_logger) << _domain << " invalid address: " << _domain;
			}
			initAddress(result);
		}else{
			init();
		}
	}

	void DnsManager::add(Dns::ptr v){
		RWMutexType::WriteLock lock(_mutex);
		_dns[v->getDomain()] = v;
	}

	Dns::ptr DnsManager::get(const std::string& domain){
		RWMutexType::WriteLock lock(_mutex);
		auto it = _dns.find(domain);
		return it == _dns.end() ? nullptr : it->second;
	}

	Routn::Address::ptr DnsManager::getAddress(const std::string& service, bool cache, uint32_t seed){
		auto dns = get(service);
		if(dns){
			return dns->get(seed);
		}

		if(cache){
			Routn::IOManager::GetThis()->schedule([service, this](){
				Dns::ptr dns = std::make_shared<Dns>(service, Dns::TYPE_DOMAIN);
				dns->refresh();
				add(dns);
			});
		}

		return Routn::Address::LookupAny(service, Routn::Socket::IPV4, Socket::TCP);
	}


	Routn::Socket::ptr DnsManager::getSocket(const std::string& service, bool cache, uint32_t seed){
		auto dns = get(service);
		if(dns){
			return dns->getSock(seed);
		}

		if(cache){
			Routn::IOManager::GetThis()->schedule([service, this](){
				Dns::ptr dns = std::make_shared<Dns>(service, Dns::TYPE_DOMAIN, 1);
				dns->refresh();
				add(dns);
			});
		}
		auto addr = Routn::Address::LookupAny(service, Routn::Socket::IPV4, Socket::TCP);
		Routn::Socket::ptr sock = Socket::CreateTCP(addr);
		if(sock->connect(addr, 20)){
			return sock;
		}
		return nullptr;
	}

	void DnsManager::init(){
		if(_refresh){
			return;
		}
		_refresh = true;
		RWMutexType::ReadLock lock(_mutex);
		std::map<std::string, Dns::ptr> dns = _dns;
		lock.unlock();
		for(auto &i : dns){
			i.second->refresh();
		}
		_refresh = false;
		_lastUpdateTime = time(0);
	}
	
	void DnsManager::start(){
		if(_timer){
			return ;
		}
		_timer = Routn::IOManager::GetThis()->addTimer(1000, std::bind(&DnsManager::init, this), true);
	}



}


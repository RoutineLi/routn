/*************************************************************************
	> File Name: NameServerProtocol.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月08日 星期日 14时35分21秒
 ************************************************************************/

#include "NameServerProtocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>

namespace Routn{
namespace Ns{

	NSNode::NSNode(const std::string& ip, uint16_t port, uint32_t weight)
		: _ip(ip)
		, _port(port)
		, _weight(weight){
		_id = GetID(ip, port);
	}
	
	uint64_t NSNode::GetID(const std::string& ip, uint16_t port){
		in_addr_t ip_addr = inet_addr(ip.c_str());
		uint64_t v = (((uint64_t)ip_addr) << 32) | port;
		return v;
	}
	
	std::ostream& NSNode::dump(std::ostream& os, const std::string& prefix){
		os << prefix << "[NSNode id = " << _id
		   			 << " ip = " << _ip
					 << " port = " << _port
					 << " weight = " << _weight
					 << "]";
		return os;
	}

	std::string NSNode::toString(const std::string &prefix){
		std::stringstream ss;
		dump(ss, prefix);
		return ss.str();
	}


	NSNodeSet::NSNodeSet(uint32_t cmd)
		: _cmd(cmd){

	}

	void NSNodeSet::add(NSNode::ptr info){
		Routn::RW_Mutex::WriteLock lock(_mutex);
		_datas[info->getId()] = info;
	}

	NSNode::ptr NSNodeSet::del(uint64_t id){
		NSNode::ptr rt;
		Routn::RW_Mutex::WriteLock lock(_mutex);
		auto it = _datas.find(id);
		if(it != _datas.end()){
			rt = it->second;
			_datas.erase(it);
		}
		return rt;
	}

	NSNode::ptr NSNodeSet::get(uint64_t id){
		Routn::RW_Mutex::ReadLock lock(_mutex);
		auto it = _datas.find(id);
		return it == _datas.end() ? nullptr : it->second;
	}

	void NSNodeSet::listAll(std::vector<NSNode::ptr>& infos){
		Routn::RW_Mutex::ReadLock lock(_mutex);
		for(auto& i : _datas){
			infos.push_back(i.second);
		}
	}

	std::ostream& NSNodeSet::dump(std::ostream& os, const std::string& prefix){
		os << prefix << "[NSNodeSet cmd = " << _cmd;
		Routn::RW_Mutex::ReadLock lock(_mutex);
		os << " size = " << _datas.size() << "]" << std::endl;
		for(auto& i : _datas){
			i.second->dump(os, prefix + "	") << std::endl;
		}
		return os;
	}

	std::string NSNodeSet::toString(const std::string& prefix){
		std::stringstream ss;
		dump(ss, prefix);
		return ss.str();
	}	

	size_t NSNodeSet::size(){
		Routn::RW_Mutex::WriteLock lock(_mutex);
		return _datas.size();
	}


	void NSDomain::add(NSNodeSet::ptr info){
		Routn::RW_Mutex::WriteLock lock(_mutex);
		_datas[info->getCmd()] = info;
	}

	void NSDomain::add(uint32_t cmd, NSNode::ptr info){
		auto ns = get(cmd);
		if(!ns){
			ns = std::make_shared<NSNodeSet>(cmd);
			add(ns);
		}
		ns->add(info);
	}

	void NSDomain::del(uint32_t cmd){
		Routn::RW_Mutex::WriteLock lock(_mutex);
		_datas.erase(cmd);
	}

	NSNode::ptr NSDomain::del(uint32_t cmd, uint64_t id){
		auto ns = get(cmd);
		if(!ns){
			return nullptr;
		}
		auto info = ns->del(id);
		if(!ns->size()){
			del(cmd);
		}
		return info;
	}

	NSNodeSet::ptr NSDomain::get(uint32_t cmd){
		Routn::RW_Mutex::ReadLock lock(_mutex);
		auto it = _datas.find(cmd);
		return it == _datas.end() ? nullptr : it->second;
	}

	void NSDomain::listAll(std::vector<NSNodeSet::ptr>& infos){
		Routn::RW_Mutex::ReadLock lock(_mutex);
		for(auto& i : _datas){
			infos.push_back(i.second);
		}
	}

	std::ostream& NSDomain::dump(std::ostream& os, const std::string& prefix){
		os << prefix << "[NSDomain name = " << _domain;
		Routn::RW_Mutex::ReadLock lock(_mutex);
		os << " cmd_size = " << _datas.size() << "]" << std::endl;
		for(auto& i : _datas){
			i.second->dump(os, prefix + "	") << std::endl;
		}
		return os;
	}

	std::string NSDomain::toString(const std::string& prefix){
		std::stringstream ss;
		dump(ss, prefix);
		return ss.str();
	}

	size_t NSDomain::size(){
		Routn::RW_Mutex::WriteLock lock(_mutex);
		return _datas.size();
	}


	void NSDomainSet::add(NSDomain::ptr info){
		Routn::RW_Mutex::WriteLock lock(_mutex);
		_datas[info->getDomain()] = info;
	}

	void NSDomainSet::del(const std::string& domain){
		Routn::RW_Mutex::WriteLock lock(_mutex);
		_datas.erase(domain);
	}

	NSDomain::ptr NSDomainSet::get(const std::string& domain, bool auto_create){
		{
			Routn::RW_Mutex::ReadLock lock(_mutex);
			auto it = _datas.find(domain);
			if(!auto_create){
				return it == _datas.end() ? nullptr : it->second;
			}
		}
		Routn::RW_Mutex::WriteLock lock(_mutex);
		auto it = _datas.find(domain);
		if(it != _datas.end()){
			return it->second;
		}
		NSDomain::ptr d = std::make_shared<NSDomain>(domain);
		_datas[domain] = d;
		return d;
	}


	void NSDomainSet::del(const std::string& domain, uint32_t cmd, uint64_t id){
		auto d = get(domain);
		if(!d){
			return ;
		}
		auto ns = d->get(cmd);
		if(!ns){
			return ;
		}
		ns->del(id);
	}

	void NSDomainSet::listAll(std::vector<NSDomain::ptr>& infos){
		Routn::RW_Mutex::ReadLock lock(_mutex);
		for(auto &i : _datas){
			infos.push_back(i.second);
		}
	}


	std::ostream& NSDomainSet::dump(std::ostream& os, const std::string& prefix){
		Routn::RW_Mutex::ReadLock lock(_mutex);
		os << prefix << "[NSDomainSet domain_size=" << _datas.size() << "]" << std::endl;
		for(auto& i : _datas) {
			os << prefix;
			i.second->dump(os, prefix + "	") << std::endl;
		}
		return os;
	}

	std::string NSDomainSet::toString(const std::string& prefix){
	    std::stringstream ss;
		dump(ss, prefix);
		return ss.str();	
	}

	void NSDomainSet::swap(NSDomainSet& ds){
		if(this == &ds){
			return ;
		}
		Routn::RW_Mutex::WriteLock lock(_mutex);
		Routn::RW_Mutex::WriteLock lock2(ds._mutex);
		_datas.swap(ds._datas);
	}

}
}


/*************************************************************************
	> File Name: NameServerModule.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月09日 星期一 01时28分34秒
 ************************************************************************/

#include "NameServerModule.h"
#include "../Log.h"
#include "../Worker.h"

namespace Routn{
namespace Ns{
	static Logger::ptr g_logger = ROUTN_LOG_NAME("system");
	uint64_t s_request_count = 0;
	uint64_t s_on_connect = 0;
	uint64_t s_on_disconnect = 0;

	NameServerModule::NameServerModule()
		:RockModule("NameServerModule", "1.0.0", ""){
		_domains = std::make_shared<NSDomainSet>();
	}

	bool NameServerModule::handleRockRequest(RockRequest::ptr request
								, RockResponse::ptr response
								, RockStream::ptr stream){
		Atomic::addFetch(s_request_count, 1);
		switch(request->getCmd()){
			case (int)NSCommand::REGISTER:
				return handleRegister(request, response, stream);
			case (int)NSCommand::QUERY:
				return handleQuery(request, response, stream);
			case (int)NSCommand::TICK:
				return handleTick(request, response, stream);
			default:
				ROUTN_LOG_WARN(g_logger) << "invalid cmd = 0x" << std::hex << request->getCmd();
				break;
		}
		return true;
	}

	bool NameServerModule::handleRockNotify(RockNotify::ptr notify
								, RockStream::ptr stream){
		return true;
	}
	
	bool NameServerModule::onConnect(Stream::ptr stream){
		Atomic::addFetch(s_on_connect, 1);
		auto rockstream = std::dynamic_pointer_cast<RockStream>(stream);
		if(!rockstream){
			ROUTN_LOG_ERROR(g_logger) << "invalid stream";
			return false;
		}
		auto addr = rockstream->getSocket()->getRemoteAddress();
		if(addr){
			ROUTN_LOG_INFO(g_logger) << "onConnect: " << *addr;
		}
		return true;
	}

	
	bool NameServerModule::onDisconnect(Stream::ptr stream){
		Atomic::addFetch(s_on_disconnect, 1);
		auto rockstream = std::dynamic_pointer_cast<RockStream>(stream);
		if(!rockstream){
			ROUTN_LOG_ERROR(g_logger) << "invalid stream";
			return false;
		}
		auto addr = rockstream->getSocket()->getRemoteAddress();
		if(addr){
			ROUTN_LOG_INFO(g_logger) << "onDisconnect: " << *addr;
		}
		set(rockstream, nullptr);
		return true;
	}

	NSClientInfo::ptr NameServerModule::get(RockStream::ptr rs){
		RW_Mutex::ReadLock lock(_mutex);
		auto it = _sessions.find(rs);
		return it == _sessions.end() ? nullptr : it->second;
	}	

	void diff(const std::map<std::string, std::set<uint32_t> >& old_value,
			const std::map<std::string, std::set<uint32_t> >& new_value,
			std::map<std::string, std::set<uint32_t> >& dels,
			std::map<std::string, std::set<uint32_t> >& news,
			std::map<std::string, std::set<uint32_t> >& comms) {
		for(auto& i : old_value) {
			auto it = new_value.find(i.first);
			if(it == new_value.end()) {
				dels.insert(i);
				continue;
			}
			for(auto& n : i.second) {
				auto iit = it->second.find(n);
				if(iit == it->second.end()) {
					dels[i.first].insert(n);
					continue;
				}
				comms[i.first].insert(n);
			}
		}

		for(auto& i : new_value) {
			auto it = old_value.find(i.first);
			if(it == old_value.end()) {
				news.insert(i);
				continue;
			}
			for(auto& n : i.second) {
				auto iit = it->second.find(n);
				if(iit == it->second.end()) {
					news[i.first].insert(n);
					continue;
				}
			}
		}
	}

	void NameServerModule::set(RockStream::ptr rs, NSClientInfo::ptr info){
		if(!rs->isConnected()){
			info = nullptr;
		}

		auto old_info = get(rs);
	
		std::map<std::string, std::set<uint32_t> > old_v;
		std::map<std::string, std::set<uint32_t> > new_v;
		std::map<std::string, std::set<uint32_t> > dels;
		std::map<std::string, std::set<uint32_t> > news;
		std::map<std::string, std::set<uint32_t> > comms;	
		if(old_info){
			old_v = old_info->_domain2cmds;
		}
		if(info){
			new_v = info->_domain2cmds;
		}
		diff(old_v, new_v, dels, news, comms);
		for(auto& i : dels){
			auto d = _domains->get(i.first);
			if(d){
				for(auto& c : i.second){
					d->del(c, old_info->_node->getId());
				}
			}
		}
		for(auto& i : news){
			auto d = _domains->get(i.first);
			if(!d){
				d = std::make_shared<NSDomain>(i.first);
				_domains->add(d);
			}
			for(auto& c : i.second){
				d->add(c, info->_node);
			}
		}
		if(!comms.empty()){
			if(old_info->_node->getWeight() != info->_node->getWeight()){
				for(auto& i : comms){
					auto d = _domains->get(i.first);
					if(!d){
						d = std::make_shared<NSDomain>(i.first);
						_domains->add(d);
					}
					for(auto& c : i.second){
						d->add(c, info->_node);
					}
				}
			}
		}

		RW_Mutex::WriteLock lock(_mutex);
		if(info){
			_sessions[rs] = info;
		}else{
			_sessions.erase(rs);
		}
	}

	void NameServerModule::setQueryDomain(RockStream::ptr rs, const std::set<std::string>& ds){

	}

	void NameServerModule::doNotify(std::set<std::string>& domains, std::shared_ptr<NotifyMessage> nty){
		RockNotify::ptr notify = std::make_shared<RockNotify>();
		notify->setNotify((int)NSNotify::NODE_CHANGE);
		notify->setAsPB(*nty);
		for(auto& i : domains){
			auto ss = getStreams(i);
			for(auto& n : ss){
				n->sendMessage(notify);
			}
		}
	}

	std::set<RockStream::ptr> NameServerModule::getStreams(const std::string& domain){
		RW_Mutex::ReadLock lock(_mutex);
		auto it = _domainToSessions.find(domain);
		return it == _domainToSessions.end() ? std::set<RockStream::ptr>() : it->second;
	}
	
	bool NameServerModule::handleQuery(RockRequest::ptr request, RockResponse::ptr response, RockStream::ptr stream){
		auto qreq = request->getAsPB<QueryRequest>();
		if(!qreq){
			ROUTN_LOG_ERROR(g_logger) << "invalid query request from: "
				<< stream->getSocket()->getRemoteAddress()->toString();
			return false;
		}
		if(!qreq->domains_size()){
			ROUTN_LOG_ERROR(g_logger) << "invalid query request from: "
				<< stream->getSocket()->getRemoteAddress()->toString()
				<< " domains is null";
		}
		std::set<NSDomain::ptr> domains;
		std::set<std::string> ds;
		for(auto& i : qreq->domains()){
			auto d = _domains->get(i);
			if(d){
				domains.insert(d);
			}
			ds.insert(i);
		}
		auto qrsp = std::make_shared<QueryResponse>();
		for(auto& i : domains){
			std::vector<NSNodeSet::ptr> nss;
			i->listAll(nss);
			for(auto& n : nss){
				auto item = qrsp->add_infos();
				item->set_domain(i->getDomain());
				item->set_cmd(n->getCmd());
				std::vector<NSNode::ptr> ns;
				n->listAll(ns);

				for(auto &x : ns){
					auto node = item->add_nodes();
					node->set_ip(x->getIp());
					node->set_port(x->getPort());
					node->set_weight(x->getWeight());
				}
			}
		}
		response->setResult(0);
		response->setResStr("ok");
		response->setAsPB(*qrsp);
		return true;
	}
	
	bool NameServerModule::handleTick(RockRequest::ptr request, RockResponse::ptr response, RockStream::ptr stream){
		return true;
	}

	bool NameServerModule::handleRegister(RockRequest::ptr request, RockResponse::ptr response, RockStream::ptr stream){
		auto rr = request->getAsPB<RegisterRequest>();
		if(!rr){
			ROUTN_LOG_ERROR(g_logger) << "invalid register request from: "
				<< stream->getSocket()->getRemoteAddress()->toString();
			return false;
		}
		auto old_value = get(stream);
		NSClientInfo::ptr new_value;
		for(int i = 0; i < rr->info_size(); ++i){
			auto& info = rr->info(i);
	#define XX(info, attr) \
			if(!info.has_##attr()){	\
				ROUTN_LOG_ERROR(g_logger) << "invalid register request from: "	\
					<< stream->getSocket()->getRemoteAddress() \
					<< " " #attr " is null"; \
				return false; \
			}
			XX(info, node);
			XX(info, domain);

			if(info.cmds_size() == 0){
				ROUTN_LOG_ERROR(g_logger) << "invalid register request from: "
					<< stream->getSocket()->getRemoteAddress()
					<< " cmds is null";
				return false;
			}
			auto &node = info.node();
			XX(node, ip);
			XX(node, port);
			XX(node, weight);

			NSNode::ptr ns_node = std::make_shared<NSNode>(node.ip(), node.port(), node.weight());
			if(!(ns_node->getId() >> 32)){
				ROUTN_LOG_ERROR(g_logger) << "invalid register request from: "
					<< stream->getSocket()->getRemoteAddress()->toString()
					<< " ip = " << node.ip() << " invalid";
				return false;
			}

			if(old_value){
				if(old_value->_node->getId() != ns_node->getId()){
					ROUTN_LOG_ERROR(g_logger) << "invalid register request from: "
						<< stream->getSocket()->getRemoteAddress()->toString()
						<< " old.ip = " << old_value->_node->getIp()
						<< " old.port = " << old_value->_node->getPort()
						<< " cur.ip = " << ns_node->getIp()
						<< " cur.port = " << ns_node->getPort();
					return false;
				}
			}
			if(new_value){
				if(new_value->_node->getId() != ns_node->getId()){
					ROUTN_LOG_ERROR(g_logger) << "invalid register request from: "
						<< stream->getSocket()->getRemoteAddress()->toString()
						<< " new.ip = " << new_value->_node->getIp()
						<< " new.port = " << new_value->_node->getPort()
						<< " cur.ip = " << ns_node->getIp()
						<< " cur.port = " << ns_node->getPort();
					return false;
				}
			}else{
				new_value = std::make_shared<NSClientInfo>();
				new_value->_node = ns_node;
			}
			for(auto& cmd : info.cmds()){
				new_value->_domain2cmds[info.domain()].insert(cmd);
			}
		}
		set(stream, new_value);
		response->setResult(0);
		response->setResStr("ok");
		return true;
	}


}
}


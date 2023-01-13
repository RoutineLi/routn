/*************************************************************************
	> File Name: NameServerClient.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月08日 星期日 15时38分12秒
 ************************************************************************/

#include "NameServerClient.h"
#include "../Log.h"
#include "../Util.h"


namespace Routn{
namespace Ns{
	static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

	NSClient::NSClient(){
		_domains = std::make_shared<Routn::Ns::NSDomainSet>();
	}

	NSClient::~NSClient(){
		ROUTN_LOG_DEBUG(g_logger) << "NSClient::~NSClient";
	}

	const std::set<std::string>& NSClient::getQueryDomains(){
		RW_Mutex::ReadLock lock(_mutex);
		return _queryDomains;
	}

	void NSClient::setQueryDomains(const std::set<std::string>& v){
		RW_Mutex::WriteLock lock(_mutex);
		_queryDomains = v;
		lock.unlock();
		onQueryDomainChange();
	}


	void NSClient::addQueryDomain(const std::string& domain){
		RW_Mutex::WriteLock lock(_mutex);
		_queryDomains.insert(domain);
		lock.unlock();
		onQueryDomainChange();
	}

	void NSClient::delQueryDomain(const std::string& domain){
		RW_Mutex::WriteLock lock(_mutex);
		_queryDomains.erase(domain);
		lock.unlock();
		onQueryDomainChange();
	}

	bool NSClient::hasQueryDomain(const std::string& domain){
		RW_Mutex::ReadLock lock(_mutex);
		return _queryDomains.count(domain) > 0;
	}

	RockResult::ptr NSClient::query(){
		RockRequest::ptr req = std::make_shared<RockRequest>();
		req->setSn(Routn::Atomic::addFetch(_sn, 1));
		req->setCmd((int)NSCommand::QUERY);
		auto data = std::make_shared<Ns::QueryRequest>();

		RW_Mutex::ReadLock lock(_mutex);
		for(auto& i : _queryDomains){
			data->add_domains(i);
		}
		if(_queryDomains.empty()){
			//0, "ok", 0, nullptr, nullptr
			auto rt = std::make_shared<RockResult>(0, "ok", 0, nullptr, nullptr);
			return rt;
		}
		lock.unlock();
		req->setAsPB(*data);

		auto rt = request(req, 1000);
		do{
			if(!rt->response){
				ROUTN_LOG_ERROR(g_logger) << "query error result = " << rt->result;
				break;
			}
			auto rsp = rt->response->getAsPB<Ns::QueryResponse>();
			if(!rsp){
				ROUTN_LOG_ERROR(g_logger) << "invalid data not QueryResponse";
				break;
			}

			NSDomainSet::ptr domains = std::make_shared<NSDomainSet>();
			for(auto& i : rsp->infos()){
				if(!hasQueryDomain(i.domain())){
					continue;
				}
				auto domain = domains->get(i.domain(), true);
				uint32_t cmd = i.cmd();

				for(auto &n : i.nodes()){
					NSNode::ptr node = std::make_shared<NSNode>(n.ip(), n.port(), n.weight());
					if(!(node->getId() >> 32)) {
						ROUTN_LOG_ERROR(g_logger) << "invalid node: "
							<< node->toString();
						continue;
					}
					domain->add(cmd, node);
				}
			}
			_domains->swap(*domains);
		}while(false);
		return rt;
	}


	void NSClient::init(){
		auto self = std::dynamic_pointer_cast<NSClient>(shared_from_this());
		setConnectCB(std::bind(&NSClient::onConnect, self, std::placeholders::_1));
		setDisConnectCB(std::bind(&NSClient::onDisConnect, self, std::placeholders::_1));
		setNotifyHandler(std::bind(&NSClient::onNotify, self, std::placeholders::_1, std::placeholders::_2));
	}

	void NSClient::uninit(){
		setConnectCB(nullptr);
		setDisConnectCB(nullptr);
		setNotifyHandler(nullptr);
		if(_timer){
			_timer->cancel();
		}
	}


	void NSClient::onQueryDomainChange(){
		if(isConnected()){
			query();
		}
	}

	bool NSClient::onConnect(AsyncSocketStream::ptr stream){
		if(_timer) {
			_timer->cancel();
		}
		auto self = std::dynamic_pointer_cast<NSClient>(shared_from_this());
		_timer = _ioMgr->addTimer(30 * 1000, std::bind(&NSClient::onTimer, self), true);
		_ioMgr->schedule(std::bind(&NSClient::query, self));
		return true;
	}

	bool NSClient::onDisConnect(AsyncSocketStream::ptr stream){
		///TODO
		return true;
	}


	/// @brief message NotifyMessage{
	///				repeated NodeInfo dels = 1;
	///				repeated NodeInfo updates = 2;
	///			}
	/// @param nty 
	/// @param stream 
	/// @return true or false
	bool NSClient::onNotify(RockNotify::ptr nty, RockStream::ptr stream){
		do {
			if(nty->getNotify() == (uint32_t)NSNotify::NODE_CHANGE) {
				auto nm = nty->getAsPB<NotifyMessage>();
				if(!nm) {
					ROUTN_LOG_ERROR(g_logger) << "invalid node_change data";
					break;
				}
				/// @brief 如果是删除节点
				for(auto& i : nm->dels()) {
					if(!hasQueryDomain(i.domain())) {
						continue;
					}
					auto domain = _domains->get(i.domain());
					if(!domain) {
						continue;
					}
					int cmd = i.cmd();
					for(auto& n : i.nodes()) {
						NSNode::ptr node = std::make_shared<NSNode>(n.ip(), n.port(), n.weight());
						domain->del(cmd, node->getId());
					}
				}
				/// @brief 如果是更新节点
				for(auto& i : nm->updates()) {
					if(!hasQueryDomain(i.domain())) {
						continue;
					}
					auto domain = _domains->get(i.domain(), true);
					int cmd = i.cmd();
					for(auto& n : i.nodes()) {
						NSNode::ptr node = std::make_shared<NSNode>(n.ip(), n.port(), n.weight());
						if(node->getId() >> 32) {
							domain->add(cmd, node);
						} else {
							ROUTN_LOG_ERROR(g_logger) << "invalid node: " << node->toString();
						}
					}
				}
			}
		} while(false);
		return true;
	}

	void NSClient::onTimer(){
		RockRequest::ptr req = std::make_shared<RockRequest>();
		req->setSn(Atomic::addFetch(_sn, 1));
		req->setCmd((uint32_t)NSCommand::TICK);
		auto rt = request(req, 1000);
		if(!rt->response){
			ROUTN_LOG_ERROR(g_logger) << "tick error result = " << rt->result;
		}
		sleep(1000);
		query();
	}

}
}


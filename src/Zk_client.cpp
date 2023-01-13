/*************************************************************************
	> File Name: Zk_client.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月09日 星期一 15时48分46秒
 ************************************************************************/

#include "Zk_client.h"

namespace Routn{
	const int ZKClient::EventType::CREATED = ZOO_CREATED_EVENT;
	const int ZKClient::EventType::DELETED = ZOO_DELETED_EVENT;
	const int ZKClient::EventType::CHANGED = ZOO_CHANGED_EVENT;
	const int ZKClient::EventType::CHILD   = ZOO_CHILD_EVENT;
	const int ZKClient::EventType::SESSION = ZOO_SESSION_EVENT;
	const int ZKClient::EventType::NOWATCHING = ZOO_NOTWATCHING_EVENT;

	const int ZKClient::FlagsType::EPHEMERAL = ZOO_EPHEMERAL;
	const int ZKClient::FlagsType::SEQUENCE  = ZOO_SEQUENCE;
	const int ZKClient::FlagsType::CONTAINER = ZOO_CONTAINER;

	const int ZKClient::StateType::EXPIRED_SESSION = ZOO_EXPIRED_SESSION_STATE;
	const int ZKClient::StateType::AUTH_FAILED = ZOO_AUTH_FAILED_STATE;
	const int ZKClient::StateType::CONNECTING = ZOO_CONNECTING_STATE;
	const int ZKClient::StateType::ASSOCIATING = ZOO_ASSOCIATING_STATE;
	const int ZKClient::StateType::CONNECTED = ZOO_CONNECTED_STATE;
	const int ZKClient::StateType::READONLY = ZOO_READONLY_STATE;
	const int ZKClient::StateType::NOTCONNECTED = ZOO_NOTCONNECTED_STATE;

	ZKClient::ZKClient()
		: _handle(nullptr)
		, _recvTimeout(0){

	}

	ZKClient::~ZKClient(){
		if(_handle){
			close();
		}
	}

	void ZKClient::onWatcher(zhandle_t* zh, int type, int stat, const char* path, void *watcherCtx){
		ZKClient* client = (ZKClient*)watcherCtx;
		client->_watcherCb(type, stat, path);
	}

	bool ZKClient::init(const std::string& hosts, int recv_timeout, watcher_callback cb, log_callback lcb){
		if(_handle){
			return true;
		}
		_hosts = hosts;
		_recvTimeout = recv_timeout;
    	_watcherCb = std::bind(cb, std::placeholders::_1,
                            std::placeholders::_2,
                            std::placeholders::_3,
                            shared_from_this());
		_logCb = lcb;
		_handle = zookeeper_init2(hosts.c_str(), &ZKClient::onWatcher, _recvTimeout, nullptr, this, 0, _logCb);
		return _handle != nullptr;
	}

	int32_t ZKClient::setServers(const std::string& hosts){
		return 0;
	}

	int32_t ZKClient::create(const std::string& path, const std::string& val, std::string& new_path
				, const struct ACL_vector* acl, int flags){
		return zoo_create(_handle, path.c_str(), val.c_str(), val.size(), acl, flags, &new_path[0], new_path.size());
	}

	int32_t ZKClient::exists(const std::string& path, bool watch, Stat* stat){
		return zoo_exists(_handle, path.c_str(), watch, stat);
	}

	int32_t ZKClient::del(const std::string& path, int version){
		return zoo_delete(_handle, path.c_str(), version);
	}

	int32_t ZKClient::get(const std::string& path, std::string& val, bool watch, Stat* stat){
		int len = val.size();
		int32_t rt = zoo_get(_handle, path.c_str(), watch, &val[0], &len, stat);
		if(rt == ZOK){
			val.erase(len);
		}
		return rt;
	}

	int32_t ZKClient::getConfig(std::string& val, bool watch, Stat* stat){
		return get(ZOO_CONFIG_NODE, val, watch, stat);
	}

	int32_t ZKClient::set(const std::string& path, const std::string& val, int version, Stat* stat){
		return zoo_set2(_handle, path.c_str(), val.c_str(), val.size(), version, stat);
	}

	int32_t ZKClient::getChildren(const std::string& path, std::vector<std::string>& val, bool watch, Stat* stat){
		String_vector strings;
		Stat tmp;
		if(stat == nullptr){
			stat = &tmp;
		}
		int32_t rt = zoo_get_children2(_handle, path.c_str(), watch, &strings, stat);
		if(rt == ZOK){
			for(int32_t i = 0; i < strings.count; ++i){
				val.push_back(strings.data[i]);
			}
			deallocate_String_vector(&strings);
		}
		return rt;
	}	

	int32_t ZKClient::close(){
		_watcherCb = nullptr;
		int32_t rt = ZOK;
		if(_handle){
			rt = zookeeper_close(_handle);
			_handle = nullptr;
		}
		return rt;
	}

	int32_t ZKClient::getState(){
		return zoo_state(_handle);
	}

	std::string ZKClient::getCurrentServer(){
		auto rt = zoo_get_current_server(_handle);
		return rt == nullptr ? "" : rt;
	}



	bool ZKClient::reconnect(){
		if(_handle){
			zookeeper_close(_handle);
		}
		_handle = zookeeper_init2(_hosts.c_str(), &ZKClient::onWatcher, _recvTimeout, nullptr, this, 0, _logCb);
		return _handle != nullptr;
	}


} 

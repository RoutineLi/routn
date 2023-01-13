/*************************************************************************
	> File Name: Zk_client.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月09日 星期一 15时48分40秒
 ************************************************************************/

#ifndef _ZK_CLIENT_H
#define _ZK_CLIENT_H

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <functional>

#ifndef THREADED
#define THREADED
#endif

#include <zookeeper/zookeeper.h>

namespace Routn{
	class ZKClient : public std::enable_shared_from_this<ZKClient>{
	public:
		class EventType{
		public:
			static const int CREATED;
			static const int DELETED;
			static const int CHANGED;
			static const int CHILD;
			static const int SESSION;
			static const int NOWATCHING;
		};
		class FlagsType{
		public:
			static const int EPHEMERAL;
			static const int SEQUENCE;
			static const int CONTAINER;
		};
		class StateType{
		public:
			static const int EXPIRED_SESSION;
			static const int AUTH_FAILED;
			static const int CONNECTING;
			static const int ASSOCIATING;
			static const int CONNECTED;
			static const int READONLY;
			static const int NOTCONNECTED;
		};

		using ptr = std::shared_ptr<ZKClient>;
		using watcher_callback = std::function<void(int type
												, int stat, const std::string& path
												, ZKClient::ptr)>;
		typedef void(*log_callback)(const char* message);

		ZKClient();
		~ZKClient();

		bool init(const std::string& hosts, int recv_timeout, watcher_callback cb, log_callback lcb = nullptr);
		int32_t setServers(const std::string& hosts);

		int32_t create(const std::string& path, const std::string& val, std::string& new_path
					, const struct ACL_vector* acl = &ZOO_OPEN_ACL_UNSAFE, int flags= 0);
		int32_t exists(const std::string& path, bool watch, Stat* stat = nullptr);
		int32_t del(const std::string& path, int version = -1);
		int32_t get(const std::string& path, std::string& val, bool watch, Stat* stat = nullptr);
		int32_t getConfig(std::string& val, bool watch, Stat* stat = nullptr);
		int32_t set(const std::string& path, const std::string& val, int version = - 1, Stat* stat = nullptr);
		int32_t getChildren(const std::string& path, std::vector<std::string>& val, bool watch, Stat* stat = nullptr);
		int32_t close();
		int32_t getState();
		std::string getCurrentServer();

		bool reconnect();
	private:
		static void onWatcher(zhandle_t* zh, int type, int stat, const char* path, void *watcherCtx);
    	typedef std::function<void(int type, int stat, const std::string& path)> watcher_callback2;
	private:
		zhandle_t* _handle;
		std::string _hosts;
		watcher_callback2 _watcherCb;
		log_callback _logCb;
		int32_t _recvTimeout;
	};
}


#endif

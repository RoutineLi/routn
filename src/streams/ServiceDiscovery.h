/*************************************************************************
	> File Name: ServiceDiscovery.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月10日 星期二 23时08分38秒
 ************************************************************************/

#ifndef _SERVICEDISCOVERY_H
#define _SERVICEDISCOVERY_H

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <string>
#include "../Thread.h"
#include "../IoManager.h"
#include "../Util.h"

#define WITH_ZK_CLIENT	1

#if WITH_ZK_CLIENT
#include "../Zk_client.h"
#endif

namespace Routn{

	/// @brief 服务封装
	class ServiceItemInfo{
	public:
		using ptr = std::shared_ptr<ServiceItemInfo>;
		static ServiceItemInfo::ptr Create(const std::string& ip_and_port, const std::string& data);

		uint64_t getId() const { return _id;}
		uint64_t getPort() const { return _port;}
		uint32_t getUpdateTime() const { return _updateTime;}
		const std::string& getIp() const { return _ip;}
		const std::string& getData() const { return _data;}
		const std::string& getType() const { return _type;}

		std::string toString() const;

		std::string getData(const std::string& key, const std::string& def = "") const;
		template<class T>
		T getDataAs(const std::string& key, const T& def = T()) const {
			return Routn::GetParamValue(_datas, key, def);
		}
		void setData(const std::map<std::string, std::string>& v) { _datas = v;}
	private:
		uint64_t 	_id;
		uint16_t 	_port;
		uint32_t 	_updateTime;
		std::string _ip;
		std::string _data;
		std::string _type;
		std::map<std::string, std::string> _datas;
	};

	/// @brief 服务发现基类
	class IServiceDiscovery{
	public:
		using ptr = std::shared_ptr<IServiceDiscovery>;
		using service_callback = std::function<void(const std::string& domain, const std::string& service
				, const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& old_value
				, const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& new_value)> ;
		virtual ~IServiceDiscovery() {}

		virtual bool doRegister() = 0;
		virtual bool doQuery() = 0;

		void registerServer(const std::string& domain, const std::string& service,
							const std::string& ip_and_port, const std::string& data);
		void queryServer(const std::string& domain, const std::string& service);
		void listServer(std::unordered_map<std::string, std::unordered_map<std::string
							, std::unordered_map<uint64_t, ServiceItemInfo::ptr>> >& infos);
		void listRegisterServer(std::unordered_map<std::string, std::unordered_map<std::string
                            ,std::unordered_map<std::string, std::string> > >& infos);
		void listQueryServer(std::unordered_map<std::string, std::unordered_set<std::string>> & infos);
		
		virtual void start() = 0;
		virtual void stop() = 0;

		void addServiceCallback(service_callback v);

		void setQueryServer(const std::unordered_map<std::string, std::unordered_set<std::string> >& v);

		const std::string& getSelfInfo() const { return _selfInfo;}
		void setSelfInfo(const std::string& v) { _selfInfo = v;}
		const std::string& getSelfData() const { return _selfData;}
		void setSelfData(const std::string& v) { _selfData = v;}

		void addParam(const std::string& key, const std::string& val);
		std::string getParam(const std::string& key, const std::string& def = "");

		std::string toString();

	protected:
		Routn::RW_Mutex _mutex;
		//domain -> [service -> [id -> ServiceItemInfo] ]
		std::unordered_map<std::string, std::unordered_map<std::string
			,std::unordered_map<uint64_t, ServiceItemInfo::ptr> > > _datas;
		//domain -> [service -> [ip_and_port -> data] ]
		std::unordered_map<std::string, std::unordered_map<std::string
			,std::unordered_map<std::string, std::string> > > _registerInfos;
		//domain -> [service]
		std::unordered_map<std::string, std::unordered_set<std::string> > _queryInfos;

		std::vector<service_callback> _cbs;

		std::string _selfInfo;
		std::string _selfData;

		std::map<std::string, std::string> _params;
	};

#if WITH_ZK_CLIENT
	class ZKServiceDiscovery : public IServiceDiscovery
							 , public std::enable_shared_from_this<ZKServiceDiscovery>{
	public:
		using ptr = std::shared_ptr<ZKServiceDiscovery>;
		ZKServiceDiscovery(const std::string& hosts);

		virtual void start();
		virtual void stop();

		virtual bool doRegister();
		virtual bool doQuery();
	private:
		void onWatch(int type, int stat, const std::string& path, ZKClient::ptr);
		void onZKConnect(const std::string& path, ZKClient::ptr client);
		void onZKChild(const std::string& path, ZKClient::ptr client);
		void onZKChanged(const std::string& path, ZKClient::ptr client);
		void onZKDeleted(const std::string& path, ZKClient::ptr client);
		void onZKExpiredSession(const std::string& path, ZKClient::ptr client);

		bool registerInfo(const std::string& domain, const std::string& service, 
						  const std::string& ip_and_port, const std::string& data);
		bool queryInfo(const std::string& domain, const std::string& service);
		bool queryData(const std::string& domain, const std::string& service);

		bool existsOrCreate(const std::string& path);
		bool getChildren(const std::string& path);
	private:
		std::string _hosts;
		ZKClient::ptr _client;
		Routn::Timer::ptr _timer;
		bool _isOnTimer = false;
	};
#endif
}


#endif

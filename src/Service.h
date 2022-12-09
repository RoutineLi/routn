/*************************************************************************
	> File Name: Service.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月28日 星期一 10时00分35秒
 ************************************************************************/

#ifndef _SERVICE_H
#define _SERVICE_H

#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include "Thread.h"
#include "IoManager.h"


///TODO
namespace Routn{

	class ServiceItemInfo{
	public:
		using ptr = std::shared_ptr<ServiceItemInfo>;
		static ServiceItemInfo::ptr Create(const std::string& addr, const std::string& data);

		uint64_t getId() const { return _id;}
		uint64_t getPort() const { return _port;}
		const std::string& getIp() const { return _ip;}
		const std::string& getData() const { return _data;}

		std::string toString() const;
	private:
		uint64_t _id;
		uint16_t _port;
		std::string _ip;
		std::string _data;
	};


	/**
	 * 服务注册基类
	 */
	class IServiceDiscovery{
	public:
		using RWMutexType = RW_Mutex;
		using ptr = std::shared_ptr<IServiceDiscovery>;
		using service_callback = std::function<void (const std::string& domain, const std::string& service
			, const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& old_val
			, const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& new_val)>;
		virtual ~IServiceDiscovery(){}

		void registerServer(const std::string& domain, const std::string& service, const std::string& addr, const std::string& data);
		void queryServer(const std::string& domain, const std::string& service);
		void listServer(std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<uint64_t, ServiceItemInfo::ptr> > >& infos);
		void listRegisterServer(std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<uint64_t, ServiceItemInfo::ptr> > >& infos);
		void listQueryServer(std::unordered_map<std::string, std::unordered_set<std::string> >& infos);
	
		virtual void start() = 0;
		virtual void stop() = 0;

		service_callback getServiceCallBack() const { return _cb;}
		void setServiceCallBack(service_callback v) { _cb = v;}

		void setQueryServer(const std::unordered_map<std::string, std::unordered_set<std::string> >& v);
	protected:
		RWMutexType _mutex;    
		//domain -> [service -> [id -> ServiceItemInfo] ]
		std::unordered_map<std::string, std::unordered_map<std::string
			,std::unordered_map<uint64_t, ServiceItemInfo::ptr> > > m_datas;
		//domain -> [service -> [ip_and_port -> data] ]
		std::unordered_map<std::string, std::unordered_map<std::string
			,std::unordered_map<std::string, std::string> > > m_registerInfos;
		//domain -> [service]
		std::unordered_map<std::string, std::unordered_set<std::string> > m_queryInfos;
		
		service_callback _cb;
	};
}
#endif

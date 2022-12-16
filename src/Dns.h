/*************************************************************************
	> File Name: Dns.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月15日 星期四 23时14分25秒
 ************************************************************************/

#ifndef _DNS_H
#define _DNS_H

#include "Singleton.h"
#include "Address.h"
#include "Socket.h"
#include "Thread.h"
#include "Timer.h"
#include <set>
#include <map>

namespace Routn{

	class Dns{
	public:
		using ptr = std::shared_ptr<Dns>;
		using RWMutexType = RW_Mutex;
		enum Type{
			TYPE_DOMAIN = 1,
			TYPE_ADDRESS = 2
		};

		Dns(const std::string& domain, int type, int32_t pool_size = 0);
		virtual ~Dns(){}
		void set(const std::set<std::string>& addrs);
		Routn::Address::ptr get(uint32_t seed = -1);
		Routn::Socket::ptr getSock(uint32_t sedd = -1);

		const std::string& getDomain() const { return _domain;}
		int getType() const { return _type;}

		std::string getCheckPath() const { return _checkPath;}
		void setCheckPath(const std::string& v) { _checkPath = v;}

		std::string toString();
		void refresh();
	public:
		struct AddressItem : public std::enable_shared_from_this<AddressItem>{
			using ptr = std::shared_ptr<AddressItem>;
			~AddressItem();
			Routn::Address::ptr addr;
			std::list<Socket*> socks;
			Routn::SpinLock _mutex;
			bool valid = false;
			uint32_t pool_size = 0;
			std::string check_path;

			bool isValid();
			bool checkValid(uint32_t timeout_ms);

			void push(Socket* sock);
			Socket::ptr pop();
			Socket::ptr getSock();

			std::string toString();
		};
	private:

		void init();

		void initAddress(const std::vector<Address::ptr>& result);
	private:
		std::string _domain;
		int _type;
		uint32_t _idx;
		uint32_t _poolsize = 0;
		std::string _checkPath;
		RWMutexType _mutex;
		std::vector<AddressItem::ptr> _address;
		std::set<std::string> _addrs;
	};


	class DnsManager{
	public:
		using RWMutexType = RW_Mutex;
		void init();
		void start();

		void add(Dns::ptr v);
		Dns::ptr get(const std::string& domain);

		/**
		 * @brief Get the Address object
		 * 
		 * @param service routn.com:80
		 * @param cache 是否需要缓存
		 * @param seed 种子
		 * @return Routn::Address::ptr 
		 */
		Routn::Address::ptr getAddress(const std::string& service, bool cache, uint32_t seed = -1);
		Routn::Socket::ptr getSocket(const std::string& service, bool cache, uint32_t seed = -1);
	private:
		RWMutexType _mutex;
		std::map<std::string, Dns::ptr> _dns;
		Routn::Timer::ptr _timer;
		bool _refresh = false;
		uint64_t _lastUpdateTime = 0;
	};

	using DnsMgr = Routn::Singleton<DnsManager>;
}

#endif

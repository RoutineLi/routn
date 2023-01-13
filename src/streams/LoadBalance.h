/*************************************************************************
	> File Name: LoadBalance.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月02日 星期一 15时12分15秒
 ************************************************************************/

#ifndef _LOADBALANCE_H
#define _LOADBALANCE_H

#include "SocketStream.h"
#include "ServiceDiscovery.h"
#include "../Thread.h"
#include "../Util.h"
#include <vector>
#include <unordered_map>

/**
 * @brief 负载均衡模块
 * 提供：轮询，权重，公平三种策略
 * 
 */

namespace Routn{

	class HolderStatsSet;
	/**
	 * @brief 权值模块，根据每个服务的使用时间
	 * ，总量，连接中的数量
	 * ，成功数量和失败数量进行计算
	 */
	class HolderStats{
	friend class HolderStatsSet;
	public:
		uint32_t getUsedTime() const { return _usedTime;}
		uint32_t getTotal() const { return _total;}
		uint32_t getTimeouts() const { return _timeouts;}
		uint32_t getOks() const { return _oks;}
		uint32_t getErrs() const { return _errs;}

		uint32_t incUsedTime(uint32_t v) { return Atomic::addFetch(_usedTime, v);}
		uint32_t incTotal(uint32_t v) { return Atomic::addFetch(_total, v);}
		uint32_t incDoing(uint32_t v) { return Atomic::addFetch(_doing, v);}
		uint32_t incTimeouts(uint32_t v) { return Atomic::addFetch(_timeouts, v);}
		uint32_t incOks(uint32_t v) { return Atomic::addFetch(_oks, v);}
		uint32_t incErrs(uint32_t v) { return Atomic::addFetch(_errs, v);}
		
		uint32_t decDoing(uint32_t v) { return Atomic::subFetch(_doing, v);}
		
		void clear();

		float getWeight(float rate = 1.0f);

		std::string toString();

		void add(const HolderStats& hs);

		uint64_t getWeight(const HolderStats& hs, uint64_t join_time);
	private:
		uint32_t _usedTime = 0;
		uint32_t _total = 0;
		uint32_t _doing = 0;
		uint32_t _timeouts = 0;
		uint32_t _oks = 0;
		uint32_t _errs = 0;
	};

	class HolderStatsSet{
	public:
		HolderStatsSet(uint32_t size = 5);
		HolderStats& get(const uint32_t& now = time(0));

		float getWeight(const uint32_t& now = time(0));
		HolderStats getTotal() const ;
	private:
		void init(const uint32_t& now);
	private:
		uint32_t _lastUpdateTime = 0;
		std::vector<HolderStats> _stats;
	};

	class LoadBalanceItem{
	public:
		using ptr = std::shared_ptr<LoadBalanceItem>;
		virtual ~LoadBalanceItem() {}

		SocketStream::ptr getStream() const { return _stream;}
		void setStream(SocketStream::ptr v) { _stream = v;}

		HolderStats& get(const uint32_t& now = time(0));
		const HolderStatsSet& getStatsSet() const { return _stats;}

		void setId(uint64_t v) { _id = v;}
		uint64_t getId() const { return _id;}

		template<class T>
		std::shared_ptr<T> getStreamAs(){
			return std::dynamic_pointer_cast<T>(_stream);
		}

		virtual int32_t getWeight() { return _weight;}
		void setWeight(int32_t v) { _weight = v;}

		virtual bool isValid();
		void close();

		std::string toString();
		uint64_t getDiscoveryTime() const { return _discoveryTime;}
	protected:
		uint64_t _id = 0;
		SocketStream::ptr _stream;
		int32_t _weight = 0;
		uint64_t _discoveryTime = time(0);
		HolderStatsSet _stats;
	};

	class ILoadBalance{
	public:
		enum Type{
			UNKNOWN = 0,
			ROUNDROBIN = 1,
			WEIGHT = 2,
			FAIR = 3
		};

		enum Error{
			NO_SERVICE = -101,
			NO_CONNECTION = -102
		};
		using ptr = std::shared_ptr<ILoadBalance>;
		virtual ~ILoadBalance(){}
		virtual LoadBalanceItem::ptr get(uint64_t v = -1) = 0;
	};

	class LoadBalance : public ILoadBalance{
	public:
		using RWMutexType = Routn::RWSpinLock;
		using ptr = std::shared_ptr<LoadBalance>;
		void add(LoadBalanceItem::ptr v);
		void del(LoadBalanceItem::ptr v);
		void set(const std::vector<LoadBalanceItem::ptr>& vs);

		LoadBalanceItem::ptr getById(uint64_t id);
		void update(const std::unordered_map<uint64_t, LoadBalanceItem::ptr>& adds
                ,std::unordered_map<uint64_t, LoadBalanceItem::ptr>& dels);
		void init();

		std::string statusString(const std::string& prefix);

		void checkInit();
	protected:
		virtual void initNolock() = 0;
	protected:
		RWMutexType _mutex;
		std::unordered_map<uint64_t, LoadBalanceItem::ptr> _datas;
		uint64_t _lastInitTime = 0;
	};


	class RoundRobinLoadBalance : public LoadBalance{
	public:
		using ptr = std::shared_ptr<RoundRobinLoadBalance>;
		virtual LoadBalanceItem::ptr get(uint64_t v = -1) override;
	protected:
		virtual void initNolock();
	protected:
		std::vector<LoadBalanceItem::ptr> _items;
	};

	class WeightLoadBalance : public LoadBalance{
	public:
		using ptr = std::shared_ptr<WeightLoadBalance>;
		virtual LoadBalanceItem::ptr get(uint64_t v = -1) override;
	protected:
		virtual void initNolock();
	private:
		int32_t getIdx(uint64_t v = -1);
	protected:
		std::vector<LoadBalanceItem::ptr> _items;
		std::vector<int64_t> _weights;
	};

	class FairLoadBalance : public WeightLoadBalance{
	public:
		using ptr = std::shared_ptr<FairLoadBalance>;
	protected:
		virtual void initNolock();
	};

	class SDLoadBalance{
	public:
		using ptr = std::shared_ptr<SDLoadBalance>;
		using stream_callback = std::function<SocketStream::ptr(const std::string& domain, const std::string& service, ServiceItemInfo::ptr info)>;
		using RWMutexType = RWSpinLock;

		SDLoadBalance(IServiceDiscovery::ptr sd);
		virtual ~SDLoadBalance() {}

		virtual void start();
		virtual void stop();
		virtual bool doQuery();
		virtual bool doRegister();

		stream_callback getCallback() const { return _callback;}
		void setCallback(stream_callback v) { _callback = v;}	
		
		LoadBalance::ptr get(const std::string& domain, const std::string& service, bool auto_create = false);

		void initConf(const std::unordered_map<std::string, std::unordered_map<std::string, std::string> >& confs);
		
		std::string statusString();

		template<class Conn>
		std::shared_ptr<Conn> getConnAs(const std::string& domain, const std::string& service, uint32_t idx = -1){
			auto lb = get(domain, service);
			if(!lb){
				return nullptr;
			}
			auto conn = lb->get(idx);
			if(!conn){
				return nullptr;
			}
			return conn->getStreamAs<Conn>();
		}
	private:
		void onServiceChange(const std::string& domain, const std::string& service
							, const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& old_value
							, const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& new_value);
		ILoadBalance::Type getType(const std::string& domain, const std::string& service);
		LoadBalance::ptr createLoadBalance(ILoadBalance::Type type);
		LoadBalanceItem::ptr createLoadBalanceItem(ILoadBalance::Type type);
	private:
		void refresh();
	protected:
		RWMutexType _mutex;
		IServiceDiscovery::ptr _sd;
		//domain -> [ service -> [ LoadBalance ] ]
		std::unordered_map<std::string, std::unordered_map<std::string, LoadBalance::ptr> > _datas;
		std::unordered_map<std::string, std::unordered_map<std::string, ILoadBalance::Type> > _types;
		//ILoadBalance::Type m_defaultType = ILoadBalance::FAIR;
		stream_callback _callback;

		Timer::ptr _timer;
		std::string _type;
		bool _isRefresh = false;
	};

}

#endif

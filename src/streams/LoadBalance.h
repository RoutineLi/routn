/*************************************************************************
	> File Name: LoadBalance.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月02日 星期一 15时12分15秒
 ************************************************************************/

#ifndef _LOADBALANCE_H
#define _LOADBALANCE_H

#include "SocketStream.h"
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

	class LoadBalanceItem{
	public:
		using ptr = std::shared_ptr<LoadBalanceItem>;
		virtual ~LoadBalanceItem();

		SocketStream::ptr getStream() const { return _stream;}
		void setStream(SocketStream::ptr v) { _stream = v;}

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


}

#endif

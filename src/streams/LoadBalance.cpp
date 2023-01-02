/*************************************************************************
	> File Name: LoadBalance.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月02日 星期一 15时12分19秒
 ************************************************************************/

#include "LoadBalance.h"
#include "../Log.h"
#include "../Worker.h"
#include "../Macro.h"
#include <cmath>

namespace Routn{
	void LoadBalanceItem::close(){
		if(_stream){
			auto stream = _stream;
			WorkerMgr::GetInstance()->schedule("service_io", [stream](){
				stream->close();
			});
		}
	}

	bool LoadBalanceItem::isValid(){
		return _stream && _stream->isConnected();
	}

	std::string LoadBalanceItem::toString(){
		std::stringstream ss;
		ss << "[Item id = " << _id
		   << " weight = " << getWeight()
		   << " discovery_time = " << TimerToString(_discoveryTime);
		if(!_stream){
			ss << " stream = null";
		}else{
			ss << " stream = [" << _stream->getSocket()->getRemoteAddress()->toString()
			   << " is_connected = " << _stream->isConnected() << "]";
		}
		return ss.str();
	}

	LoadBalanceItem::ptr LoadBalance::getById(uint64_t id){
		RWMutexType::ReadLock lock(_mutex);
		auto it = _datas.find(id);
		return it == _datas.end() ? nullptr : it->second;
	}

	void LoadBalance::add(LoadBalanceItem::ptr v){
		RWMutexType::WriteLock lock(_mutex);
		_datas[v->getId()] = v;
		initNolock();
	}

	void LoadBalance::del(LoadBalanceItem::ptr v){
		RWMutexType::WriteLock lock(_mutex);
		_datas.erase(v->getId());
		initNolock();
	}

	void LoadBalance::update(const std::unordered_map<uint64_t, LoadBalanceItem::ptr>& adds
                ,std::unordered_map<uint64_t, LoadBalanceItem::ptr>& dels){
		RWMutexType::WriteLock lock(_mutex);
		for(auto& i : dels){
			auto it = _datas.find(i.first);
			if(it != _datas.end()){
				i.second = it->second;
				_datas.erase(it);
			}
		}
		for(auto& i : adds){
			_datas[i.first] = i.second;
		}
		initNolock();
	}

	void LoadBalance::set(const std::vector<LoadBalanceItem::ptr> &vs){
		RWMutexType::WriteLock lock(_mutex);
		_datas.clear();
		for(auto& i : vs){
			_datas[i->getId()] = i;
		}
		initNolock();
	}

	void LoadBalance::init(){
		RWMutexType::WriteLock lock(_mutex);
		initNolock();
	}

	std::string LoadBalance::statusString(const std::string& prefix){
		RWMutexType::ReadLock lock(_mutex);
		decltype(_datas) datas = _datas;
		lock.unlock();
		std::stringstream ss;
		ss << prefix << "init_time: " << TimerToString(_lastInitTime / 1000) << std::endl;
		for(auto& i : datas){
			ss << prefix << i.second->toString() << std::endl;
		}
		return ss.str();
	}

	void LoadBalance::checkInit(){
		uint64_t ts = GetCurrentMs();
		if(ts - _lastInitTime > 500){
			init();
			_lastInitTime = ts;
		}
	}

	void RoundRobinLoadBalance::initNolock(){
		decltype(_items) items;
		for(auto &i : _datas){
			if(i.second->isValid()){
				items.push_back(i.second);
			}
		}
		items.swap(_items);
	}

	LoadBalanceItem::ptr RoundRobinLoadBalance::get(uint64_t v){
		RWMutexType::ReadLock lock(_mutex);
		if(_items.empty()){
			return nullptr;
		}
		uint32_t r = (v == (uint64_t)-1 ? rand() : v) % _items.size();
		for(size_t i = 0; i < _items.size(); ++i){
			auto& h = _items[(r + i) % _items.size()];
			if(h->isValid()){
				return h;
			}
		}
		return nullptr;
	}

	void WeightLoadBalance::initNolock(){
		decltype(_items) items;
		for(auto&i : _datas){
			if(i.second->isValid()){
				items.push_back(i.second);
			}
		}
		items.swap(_items);

		int64_t total = 0;
		_weights.resize(_items.size());
		for(size_t i = 0; i < _items.size(); ++i){
			total += _items[i]->getWeight();
			_weights[i] = total;
		}
	}

	LoadBalanceItem::ptr WeightLoadBalance::get(uint64_t v){
		RWMutexType::ReadLock lock(_mutex);
		int32_t idx = getIdx(v);
		if(idx == -1){
			return nullptr;
		}		
		for(size_t i = 0; i < _items.size(); ++i){
			auto &h = _items[(idx + i) % _items.size()];
			if(h->isValid()){
				return h;
			}
		}
		return nullptr;
	}

	int32_t WeightLoadBalance::getIdx(uint64_t v){
		if(_weights.empty()){
			return -1;
		}
		int64_t total = *_weights.rbegin();
		uint64_t dis = (v == (uint64_t)-1 ? rand() : v) % total;
		auto it = std::upper_bound(_weights.begin(), _weights.end(), dis);
		ROUTN_ASSERT(it != _weights.end());
		return std::distance(_weights.begin(), it);
	}

	void FairLoadBalance::initNolock(){
		decltype(_items) items;
		for(auto& i : _datas){
			if(i.second->isValid()){
				items.push_back(i.second);
			}
		}
		items.swap(_items);

		// int64_t total = 0;
		// _weights.resize(_items.size());

		///TODO
	}
}


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

	static Logger::ptr g_logger = ROUTN_LOG_NAME("system");

	void HolderStats::clear(){
		_usedTime = 0;
		_total = 0;
		_doing = 0;
		_timeouts = 0;
		_oks = 0;
		_errs = 0;
	}

	float HolderStats::getWeight(float rate){
		float base = _total + 20;
		return std::min((_oks * 1.0 / (_usedTime + 1)) * 2.0, 50.0)
			* (1 - 4.0 * _timeouts / base)
			* (1 - 1 * _doing / base)
			* (1 - 10.0 * _errs / base) * rate;
	}

	std::string HolderStats::toString(){
		std::stringstream ss;
		ss << "[Stat total = " << _total
		   << " used_time = " << _usedTime
		   << " doing = " << _doing
		   << " timeouts = " << _timeouts
		   << " oks = " << _oks
		   << " errs = " << _errs 
		   << " oks_rate=" << (_total ? (_oks * 100.0 / _total) : 0)
		   << " errs_rate=" << (_total ? (_errs * 100.0 / _total) : 0)
		   << " avg_used=" << (_oks ? (_usedTime * 1.0 / _oks) : 0)
		   << " weight=" << getWeight(1)
		   << "]";
		return ss.str();
	}

	void HolderStats::add(const HolderStats& hs){
		this->_usedTime += hs._usedTime;
		this->_doing += hs._doing;
		this->_errs += hs._errs;
		this->_oks += hs._oks;
		this->_timeouts += hs._timeouts;
		this->_total += hs._total;
	}

	uint64_t HolderStats::getWeight(const HolderStats& hs, uint64_t join_time){
		if(hs._total <= 0){
			return 100;
		}
		float all_avg_cost = hs._usedTime * 1.0 / hs._total;
		float cost_weight = 1.0;
		float err_weight = 1.0;
		float timeout_weight = 1.0;
		float doing_weight = 1.0;

		float time_weight = 1.0;
		int64_t time_diff = time(0) - join_time;
		if(time_diff < 180){
			time_weight = std::min(0.1, time_diff / 180.0);
		}

		if(_total > 10){
			cost_weight = 2 - std::min(1.9, (_usedTime * 1.0 / _total) / all_avg_cost);
			err_weight = 1 - std::min(0.9, _errs * 5.0 / _total);
			timeout_weight = 1 - std::min(0.9, _timeouts * 2.5 / _total);
			doing_weight = 1 - std::min(0.9, _doing * 1.0 / _total);
		}
		return std::min(1, (int)(200 * cost_weight * err_weight * timeout_weight * doing_weight * time_weight));
	}

	HolderStatsSet::HolderStatsSet(uint32_t size){
		_stats.resize(size);
	}

	HolderStats& HolderStatsSet::get(const uint32_t& now){
		init(now);
		return _stats[now % _stats.size()];
	}

	float HolderStatsSet::getWeight(const uint32_t& now){
		init(now);
		float v = 0;
		for(size_t i = 1; i < _stats.size(); ++i){
			v += _stats[(now - i) % _stats.size()].getWeight(1 - 0.1 * i);
		}
		return v;
	}

	HolderStats HolderStatsSet::getTotal() const {
		HolderStats rt;
		for(auto& i : _stats){
	#define XX(f) rt.f += i.f
			XX(_usedTime);
			XX(_total);
			XX(_doing);
			XX(_timeouts);
			XX(_oks);
			XX(_errs);
	#undef XX
		}
		return rt;
	}

	void HolderStatsSet::init(const uint32_t& now){
		if(_lastUpdateTime < now){
			for(uint32_t t = _lastUpdateTime + 1, i = 0; 
					t <= now && i < _stats.size(); ++t, ++i){
				_stats[t % _stats.size()].clear();
			}
			_lastUpdateTime = now;
		}
	}

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

	HolderStats& LoadBalanceItem::get(const uint32_t& now){
		return _stats.get(now);
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

		int64_t total = 0;
		_weights.resize(_items.size());

		HolderStats total_stats;
		std::vector<HolderStats> stats;
		stats.resize(_items.size());
		for(size_t i = 0; i < _items.size(); ++i){
			stats[i] = _items[i]->getStatsSet().getTotal();
			total_stats.add(stats[i]);
		}

		for(size_t i = 0; i < stats.size(); ++i){
			auto w = stats[i].getWeight(total_stats, _items[i]->getDiscoveryTime());
			_items[i]->setWeight(w);
			total += w;
			_weights[i] = total;
		}
		///TODO
	}

	SDLoadBalance::SDLoadBalance(IServiceDiscovery::ptr sd)
		: _sd(sd){
		_sd->addServiceCallback(std::bind(&SDLoadBalance::onServiceChange, this
						, std::placeholders::_1
						, std::placeholders::_2
						, std::placeholders::_3
						, std::placeholders::_4));
	}

	void SDLoadBalance::start(){
		if(_timer){
			return ;
		}
		_timer = IOManager::GetThis()->addTimer(500, std::bind(&SDLoadBalance::refresh, this), true);
		_sd->start();
	}

	void SDLoadBalance::stop(){
		if(!_timer){
			return ;
		}
		_timer->cancel();
		_timer = nullptr;
		_sd->stop();
	}

	bool SDLoadBalance::doQuery(){
		bool rt = _sd->doQuery();
		return rt;
	}

	bool SDLoadBalance::doRegister(){
		return _sd->doRegister();
	}

	LoadBalance::ptr SDLoadBalance::get(const std::string& domain, const std::string& service, bool auto_create){
		do{
			RWMutexType::ReadLock lock(_mutex);
			auto it = _datas.find(domain);
			if(it == _datas.end()){
				break;
			}
			auto iit = it->second.find(service);
			if(iit == it->second.end()){
				break;
			}
			return iit->second;
		}while(0);

		if(!auto_create){
			return nullptr;
		}

		auto type = getType(domain, service);
		auto lb = createLoadBalance(type);
		RWMutexType::WriteLock lock(_mutex);
		_datas[domain][service] = lb;
		lock.unlock();
		return lb;
	}

	void SDLoadBalance::initConf(const std::unordered_map<std::string, std::unordered_map<std::string, std::string> >& confs){
		decltype(_types) types;
		std::unordered_map<std::string, std::unordered_set<std::string> > query_infos;
		for(auto& i : confs){
			for(auto& n : i.second){
				ILoadBalance::Type t = ILoadBalance::FAIR;
				if(n.second == "round_robin"){
					t = ILoadBalance::ROUNDROBIN;
				}else if(n.second == "weight"){
					t = ILoadBalance::WEIGHT;
				}else if(n.second == "fair"){
					t = ILoadBalance::FAIR;
				}
				types[i.first][n.first] = t;
				query_infos[i.first].insert(n.first);
			}
		}
		_sd->setQueryServer(query_infos);
		RWMutexType::WriteLock lock(_mutex);
		types.swap(_types);
		lock.unlock();
	}


	std::string SDLoadBalance::statusString(){
		RWMutexType::ReadLock lock(_mutex);
		decltype(_datas) datas = _datas;
		lock.unlock();
		std::stringstream ss;
		for(auto& i : datas){
			ss << i.first << ":" << std::endl;
			for(auto& n : i.second){
				ss << "\t" << n.first << ":" << std::endl;
				ss << n.second->statusString("\t\t") << std::endl;
			}
		}
		return ss.str();
	}

	void SDLoadBalance::onServiceChange(const std::string& domain, const std::string& service
					, const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& old_value
					, const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& new_value){
		auto type = getType(domain, service);
		if(type == ILoadBalance::UNKNOWN){
			return ;
		}

		std::unordered_map<uint64_t, ServiceItemInfo::ptr> add_vals;
		std::unordered_map<uint64_t, LoadBalanceItem::ptr> del_infos;

		for(auto& i : old_value){
			if(new_value.find(i.first) == new_value.end()){
				del_infos[i.first];
			}
		}
		for(auto& i : new_value){
			if(old_value.find(i.first) == old_value.end()){
				add_vals.insert(i);
			}
		}
		std::unordered_map<uint64_t, LoadBalanceItem::ptr> add_infos;
		for(auto& i : add_vals){
			auto stream = _callback(domain, service, i.second);
			if(!stream){
				ROUTN_LOG_ERROR(g_logger) << "create stream fail, " << i.second->toString();
				continue;
			}
			LoadBalanceItem::ptr lditem = createLoadBalanceItem(type);
			lditem->setId(i.first);
			lditem->setStream(stream);

			add_infos[i.first] = lditem;
		}
		if(!add_infos.empty() || !del_infos.empty()){
			auto lb = get(domain, service, true);
			lb->update(add_infos, del_infos);
			for(auto& i : del_infos){
				if(i.second){
					i.second->close();
				}
			}
		}
	}

	ILoadBalance::Type SDLoadBalance::getType(const std::string& domain, const std::string& service){
		RWMutexType::ReadLock lock(_mutex);
		auto it = _types.find(domain);
		if(it == _types.end()){
			return ILoadBalance::UNKNOWN;
		}
		auto iit = it->second.find(service);
		if(iit == it->second.end()){
			iit = it->second.find("all");
			if(iit != it->second.end()){
				return iit->second;
			}
			return ILoadBalance::UNKNOWN;
		}
		return iit->second;
	}

	LoadBalance::ptr SDLoadBalance::createLoadBalance(ILoadBalance::Type type){
		if(type == ILoadBalance::ROUNDROBIN) {
			return std::make_shared<RoundRobinLoadBalance>();
		} else if(type == ILoadBalance::WEIGHT) {
			return std::make_shared<WeightLoadBalance>();
		} else if(type == ILoadBalance::FAIR) {
			return std::make_shared<FairLoadBalance>();
		}
		return nullptr;
	}

	LoadBalanceItem::ptr SDLoadBalance::createLoadBalanceItem(ILoadBalance::Type type){
		LoadBalanceItem::ptr item;
		if(type == ILoadBalance::ROUNDROBIN) {
			item = std::make_shared<LoadBalanceItem>();
		} else if(type == ILoadBalance::WEIGHT) {
			item = std::make_shared<LoadBalanceItem>();
		} else if(type == ILoadBalance::FAIR) {
			item = std::make_shared<LoadBalanceItem>();
		}
		return item;
	}

	void SDLoadBalance::refresh(){
		if(_isRefresh){
			return ;
		}
		_isRefresh = true;

		RWMutexType::ReadLock lock(_mutex);
		auto datas = _datas;
		lock.unlock();

		for(auto& i : datas){
			for(auto& n : i.second){
				n.second->checkInit();
			}
		}
		_isRefresh = false;
	}
}


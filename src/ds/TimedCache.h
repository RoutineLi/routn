/*************************************************************************
	> File Name: TimedCache.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月23日 星期五 17时48分25秒
 ************************************************************************/

#ifndef _TIMEDCACHE_H
#define _TIMEDCACHE_H


#include "Cache.h"
#include "../Thread.h"
#include "../Util.h"
#include <set>
#include <unordered_map>

/**
 * @brief 定时器缓存，可设置超时
 * 
 */

namespace Routn{
namespace Ds{

	template<class K, class V, class RWMutexType = Routn::RW_Mutex>
	class Timed_Cache{
	private:
		struct Item{
			Item(const K& k, const V& v, const uint64_t& t)
				: key(k)
				, val(v)
				, ts(t){

			}
			K key;
			mutable V val;
			uint64_t ts;

			bool operator<(const Item& oth) const {
				if(ts != oth.ts){
					return ts < oth.ts;
				}
				return key < oth.key;
			}
		};
	public:
		using ptr = std::shared_ptr<Timed_Cache>;
		using item_type = Item;
		using set_type = std::set<item_type>;
		using map_type = std::unordered_map<K, typename set_type::iterator>;
		using prune_callback = std::function<void(const K&, const V&)>; 

		Timed_Cache(size_t max_size = 0, size_t elasticity = 0, CacheStatus* status = nullptr)
			: _maxSize(max_size)
			, _elasticity(elasticity)
			, _status(status){
				if(_statusOwner == false){
					_status = new CacheStatus;
					_statusOwner = true;
				}
		}

		~Timed_Cache(){
			if(_statusOwner && _status){
				delete _status;
			}
		}

		void set(const K& k, const V& v, uint64_t expired){
			_status->incSet();
			typename RWMutexType::WriteLock lock(_mutex);
			auto it = _cache.find(k);
			if(it != _cache.end()){
				_cache.erase(it);
			}
			auto sit = _timed.emplace(Item(k, v, expired + Routn::GetCurrentMs()));
			_cache.emplace(std::make_pair(k, sit.first));
			prune();
		}

		bool get(const K& k , V& v){
			_status->incGet();
			typename RWMutexType::ReadLock lock(_mutex);
			auto it = _cache.find(k);
			if(it == _cache.end()){
				return false;
			}
			v = it->second->val;
			lock.unlock();
			_status->incHit();
			return true;
		}

		V get(const K& k){
			_status->incGet();
			typename RWMutexType::ReadLock lock(_mutex);
			auto it = _cache.find(k);
			if(it == _cache.end()){
				return V();
			}
			auto v = it->second->val;
			lock.unlock();
			_status->incHit();
			return v;
		}

		bool del(const K& k){
			_status->incDel();
			typename RWMutexType::WriteLock lock(_mutex);
			auto it = _cache.find(k);
			if(it == _cache.end()){
				return false;
			}
			_timed.erase(it->second);
			_cache.erase(it);
			lock.unlock();
			_status->incHit();
		}

		bool expired(const K& k, const uint64_t& ts){
			typename RWMutexType::WriteLock lock(_mutex);
			auto it = _cache.find(k);
			if(it == _cache.end()) 
				return false;
			uint64_t tts = ts + Routn::GetCurrentMs();
			if(it->second->ts == tts){
				return true;
			}
			auto item = *it->second;
			_timed.erase(it->second);
			auto iit = _timed.insert(item);
			it->second = iit.first;
			return true;
		}

		bool exists(const K& k){
			typename RWMutexType::ReadLock lock(_mutex);
			return _cache.find(k) != _cache.end();
		}

		size_t size(){
			typename RWMutexType::ReadLock lock(_mutex);
			return _cache.size();
		}

		size_t empty(){
			typename RWMutexType::ReadLock lock(_mutex);
			return _cache.empty();
		}

		bool clear(){
			typename RWMutexType::ReadLock lock(_mutex);
			_timed.clear();
			_cache.clear();
			return true;
		}

		void setMaxSize(const size_t& v) { _maxSize = v;}
		void setElasticity(const size_t& v) { _elasticity = v;}

		size_t getMaxSize() const { return _maxSize;}
		size_t getElasticity() const { return _elasticity;}
		size_t getMaxAllowedSize() const { return _maxSize + _elasticity;}

		template<class F>
		void foreach(F& f){
			typename RWMutexType::ReadLock lock(_mutex);
			std::for_each(_cache.begin(), _cache.end(), f);
		}

		void setPruneCallback(prune_callback cb) { _callback = cb;}

		std::string toStatusString(){
			std::stringstream ss;
			ss << (_status ? _status->toString() : "[no status]")
			   << " total = " << size();
			return ss.str();
		}

		CacheStatus* getStatus() const { return _status;}

		void setStatus(CacheStatus* v, bool owner = false){
			if(_statusOwner && _status){
				delete _status;
			}
			_status = v;
			_statusOwner = owner;

			if(!_status){
				_status = new CacheStatus;
				_statusOwner = true;
			}
		} 

		size_t checkTimeout(const uint64_t& ts = Routn::GetCurrentMs()){
			size_t size = 0;
			typename RWMutexType::ReadLock lock(_mutex);
			for(auto it = _timed.begin(); it != _timed.end(); ++it){
				if(it->ts <= ts){
					if(_callback){
						_callback(it->key, it->val);
					}
					_cache.erase(it->key);
					_timed.erase(it++);
					++size;
				}else{
					break;
				}
			}
			return size;
		}
	protected:
		size_t prune(){
			if(_maxSize == 0 || _cache.size() < getMaxAllowedSize()){
				return 0;
			}
			size_t count = 0;
			while(_cache.size() > _maxSize){
				auto it = _timed.begin();
				if(_callback){
					_callback(it->key, it->val);
				}
				_cache.erase(it->key);
				_timed.erase(it);
				++count;
			}
			_status->incPrune(count);
			return count;
		}
	private:
		RWMutexType _mutex;
		uint64_t _maxSize;
		uint64_t _elasticity;
		CacheStatus* _status = nullptr;
		map_type _cache;
		set_type _timed;
		prune_callback _callback;
		bool _statusOwner = false;
	};

	template<class K, class V, class RWMutexType = Routn::RW_Mutex, class Hash = std::hash<K> >
	class HashTimed_Cache{
	public:
		using ptr = std::shared_ptr<HashTimed_Cache>;
		using cache_type = Timed_Cache<K, V, RWMutexType>;

		HashTimed_Cache(size_t bucket, size_t max_size, size_t elasticity)
			: _bucket(bucket){
			_datas.resize(bucket);

			size_t pre_max_size = std::ceil(max_size * 1.0 / bucket);
			size_t pre_elasticity = std::ceil(elasticity * 1.0 / bucket);
			_maxSize = pre_max_size * bucket;
			_elasticity = pre_elasticity * bucket;

			for(size_t i = 0; i < bucket; ++i){
				_datas[i] = new cache_type(pre_max_size, pre_elasticity, &_status);
			}
		}

		~HashTimed_Cache(){
			for(size_t i = 0; i < _datas.size(); ++i){
				delete _datas[i];
			}
		}

		void set(const K& k, const V& v, uint64_t expired){
			_datas[_hash(k % _bucket)]->set(k, v, expired);
		}

		bool expired(const K& k, const uint64_t& ts){
			return _datas[_hash(k) % _bucket]->expired(k, ts);
		}

		bool get(const K& k, V& v){
			return _datas[_hash(k) % _bucket]->get(k);
		}

		V get(const K& k){
			return _datas[_hash(k) % _bucket]->get(k);
		}

		bool del(const K& k){
			return _datas[_hash(k) % _bucket]->del(k);
		}

		bool exists(const K& k){
			return _datas[_hash(k) % _bucket]->exists(k);
		}

		size_t size(){
			size_t total = 0;
			for(auto& i : _datas){
				total += i->size();
			}
			return total;
		}

		bool empty(){
			for(auto& i : _datas){
				if(!i->empty()){
					return false;
				}
			}
			return true;
		}

		void clear(){
			for(auto& i : _datas){
				i->clear();
			}
		}

		size_t getMaxSize() const { return _maxSize;}
		size_t getElasticity() const { return _elasticity;}
		size_t getMaxAllowedSize() const { return _maxSize + _elasticity;}
		size_t getBucket() const { return _bucket;}

		void setMaxSize(const size_t& v){
			size_t pre_max_size = std::ceil(v * 1.0 / _bucket);
			_maxSize = pre_max_size * _bucket;
			for(auto& i : _datas){
				i->setMaxSize(pre_max_size);
			}
		}
		void setElasticity(const size_t& v){
			size_t pre_elasticity = std::ceil(v * 1.0 / _bucket);
			_elasticity = pre_elasticity * _bucket;
			for(auto& i : _datas){
				i->setElasticity(pre_elasticity);
			}
		}

		template<class F>
		void foreach(F& f){
			for(auto& i : _datas){
				i->foreach(f);
			}
		}

		void setPruneCallback(typename cache_type::prune_callback cb){
			for(auto& i : _datas){
				i->setPruneCallback(cb);
			}
		}

		CacheStatus* getStatus(){
			return &_status;
		}

		std::string toStatusString(){
			std::stringstream ss;
			ss << _status.toString() << " total = " << size();
			return ss.str();
		}

		size_t checkTimeout(const uint64_t& ts = Routn::GetCurrentMs()){
			size_t size = 0;
			for(auto& i : _datas){
				size += i->checkTimeout(ts);
			}
			return size;
		}
	private:
		std::vector<cache_type*> _datas;
		size_t _maxSize;
		size_t _bucket;
		size_t _elasticity;
		Hash _hash;
		CacheStatus _status;
	};

}
}

#endif

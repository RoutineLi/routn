/*************************************************************************
	> File Name: LRUCache.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月22日 星期四 23时55分20秒
 ************************************************************************/

#ifndef _LRUCACHE_H
#define _LRUCACHE_H

#include <algorithm>
#include <list>
#include <vector>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <sstream>
#include "Cache.h"
#include "../Thread.h"

/**
 * @brief LRU缓存类，常用的缓存淘汰策略(Thread-Safety)
 * 
 */

namespace Routn{
namespace Ds{

	template<class K, class V, class MutexType = Routn::Mutex>
	class LRU_Cache{
	public:
		using ptr = std::shared_ptr<LRU_Cache>;
		
		using item_type = std::pair<K, V>;
		using list_type = std::list<item_type>;
		using map_type = std::unordered_map<K, typename list_type::iterator>;
		using prune_callback = std::function<void(const K&, const V&)>;

		LRU_Cache(size_t max_size = 0, size_t elasticity = 0, CacheStatus* status = nullptr)
				: _maxSize(max_size)
				, _elasticity(elasticity)
				, _status(status){
			if(_status == nullptr){
				_status = new CacheStatus;
				_statusOwner = true;
			}
		}

		~LRU_Cache(){
			if(_statusOwner && _status){
				delete _status;
			}
		}

		void set(const K& k, const V& v){
			_status->incSet();						//记录Set操作数
			typename MutexType::Lock lock(_mutex);
			auto it = _cache.find(k);
			if(it != _cache.end()){
				it->second->second = v;				//更新数据
				_keys.splice(_keys.begin(), _keys, it->second);	//刷新旧的数据到双向链表前列
				return ;
			}
	        _keys.push_front({k, v});
			_cache.insert({k, _keys.begin()});
			prune();
		}

		bool get(const K& k, V& v){
			_status->incGet();			//记录Get操作数
			typename MutexType::Lock lock(_mutex);
			auto it = _cache.find(k);
			if(it != _cache.end()){		
				_keys.splice(_keys.begin(), _keys, it->second);	//刷新数据到双向链表前列
				v = it->second->second;		//赋值
				lock.unlock();
				_status->incHit();		//记录缓存命中
				return true;
			}
			return false;
		}

		V get(const K& k){
			_status->incGet();
			typename MutexType::Lock lock(_mutex);
			auto it = _cache.find(k);
			if(it == _cache.end()) {
				return V();
			}
			_keys.splice(_keys.begin(), _keys, it->second);
			auto v = it->second->second;
			lock.unlock();
			_status->incHit();
			return v;
		}

		bool del(const K& k){
			_status->incDel();
			typename MutexType::Lock lock(_mutex);
			auto it = _cache.find(k);
			if(it == _cache.end()){
				return false;
			}
			_keys.erase(it->second);
			_cache.erase(it);
			return true;
		}

		bool exists(const K& k){
			typename MutexType::Lock lock(_mutex);
			return _cache.find(k) != _cache.end();
		}

		size_t size(){
			typename MutexType::Lock lock(_mutex);
			return _cache.size();
		}

		bool empty(){
			typename MutexType::Lock lock(_mutex);
			return _cache.empty();
		}
		
		bool clear(){
			typename MutexType::Lock lock(_mutex);
			_cache.clear();
			_keys.clear();
			return true;
		}

		void setMaxSize(const size_t& v){ _maxSize = v;}
		void setElasticity(const size_t& v) { _elasticity = v;}

		size_t getMaxSize() const { return _maxSize;}
		size_t getElasticity() const { return _elasticity;}
		size_t getMaxAllowedSize() const { return _maxSize + _elasticity;}

		template<class F>
		void foreach(F& f){
			typename MutexType::Lock lock(_mutex);
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

			if(_status == nullptr){
				_status = new CacheStatus;
				_statusOwner = true;
			}
		}

	protected:
		size_t prune(){
			if(_maxSize == 0 || _cache.size() < getMaxAllowedSize()){
				return 0;
			}
			size_t count = 0;
			while(_cache.size() > _maxSize){
				auto &back = _keys.back();
				if(_callback){
					_callback(back.first, back.second);
				}
				_cache.erase(back.first);
				_keys.pop_back();
				++count;
			}
			_status->incPrune(count);
			return count;
		}

	private:
		MutexType _mutex;			//锁
		map_type _cache;			//LRU主体哈希表
		list_type _keys;			//LRU双向链表
		size_t _maxSize;			//最大容量
		size_t _elasticity;			//弹性度
		prune_callback _callback;	
		CacheStatus* _status = nullptr;
		bool _statusOwner = false;
	};


	template<class K, class V, class MutexType = Routn::Mutex
		, class Hash = std::hash<K> >
	class HashLRU_Cache{
	public:
		using ptr = std::shared_ptr<HashLRU_Cache>;
		using cache_type = LRU_Cache<K, V, MutexType>;

		HashLRU_Cache(size_t bucket, size_t max_size, size_t elasticity)
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
		
		~HashLRU_Cache(){
			for(size_t i = 0; i < _datas.size(); ++i){
				delete _datas[i];
			}
		}

		void set(const K& k, const V& v){
			_datas[_hash(k) % _bucket]->set(k, v);
		}

		bool get(const K& k, V& v){
			return _datas[_hash(k) % _bucket]->get(k, v);
		}

		V get(const K& k){
			return _datas[_hash(k) % _bucket]->get(k);
		}

		bool del(const K& k){
			return _datas[_hash(k) % _bucket]->get(k);
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
			for(auto &i : _datas){
				i->setMaxSize(pre_max_size);
			}
		}

		void setElasticity(const size_t& v){
			size_t pre_elasticity = std::ceil(v * 1.0 / _bucket);
			_elasticity = pre_elasticity * _bucket;
			for(auto &i : _datas){
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

		std::string toStatusString() {
			std::stringstream ss;
			ss << _status.toString() << " total = " << size();
			return ss.str();
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

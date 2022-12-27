/*************************************************************************
	> File Name: TimedLRUCache.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月23日 星期五 19时46分53秒
 ************************************************************************/

#ifndef _TIMEDLRUCACHE_H
#define _TIMEDLRUCACHE_H

#include "Cache.h"
#include "../Thread.h"
#include "../Util.h"
#include <set>
#include <list>
#include <unordered_map>

/**
 * @brief 定时LRU缓存
 * 
 */

namespace Routn{
namespace Ds{

	template<class K, class V, class MutexType = Routn::Mutex>
	class TimedLRU_Cache{
	private:
		struct Item{
			Item(const K& k, const V& v, const uint64_t& t)
				: key(k), val(v), ts(t){}
			K key;
			mutable V val;
			uint64_t ts;

			bool operator<(const Item& oth) const {
				return key < oth.key;
			}
		};
	public:
		using ptr = std::shared_ptr<TimedLRU_Cache>;
		using item_type = Item;
		using list_type = std::list<item_type>;
		using value_type = typename list_type::iterator;
		using map_type = std::unordered_map<K, value_type> map_type;
		using prune_callback = std::function<void(const K&, const V&)> prune_callback;
	
	private:
		struct ItemTimeOP{
			bool operator()(const value_type& a, const value_type& b) const {
				if(a == b){
					return false;
				}
				if(a->ts != b->ts){
					return a->ts < b->ts;
				}
				return a->key < b->key;
			}
		};
	public:
		using set_type = std::set<value_type, ItemTimeOP>;

	private:
		MutexType _mutex;
		map_type _cache;
		list_type _keys;
		set_type _timed;
		size_t _maxSize;
		size_t _elasticity;
		prune_callback _callback;
		CacheStatus* _status = nullptr;
		bool _statusOwner = false;
	};


	template<class K, class V, class MutexType = Routn::Mutex, class Hash = std::hash<K>>
	class HashTimedLRU_Cache{
	public:
		using ptr = std::shared_ptr<HashTimedLRU_Cache>;
		using cache_type = TimedLRU_Cache<K, V, MutexType>;

		HashTimedLRU_Cache(size_t bucket, size_t max_size, size_t elasticity)
			: _bucket(bucket){
			_datas.resize(bucket);

			size_t pre_max_size = std::ceil(max_size * 1.0 / bucket);
			size_t pre_elasticity = std::ceil(elasticity * 1.0 / bucket);
			_maxSize = pre_max_size * bucket;
			_elasticity = pre_elasticity * bucket;

			for(size_t i = 0; i < bucket; ++i)
				_datas[i] = new cache_type(pre_max_size, pre_elasticity, &_status);
		}

		~HashTimedLRU_Cache(){
			for(size_t i = 0; i < _datas.size(); ++i){
				delete _datas[i];
			}
		}

		void set(const K& k, const V& v, uint64_t expired){
			_datas[_hash(k) % _bucket]->set(k, v, expired);
		}

		bool expired(const K &k,  const uint64_t& ts){
			return _datas[_hash(k) % _bucket]->expired(k, ts);
		}

		bool get(const K& k, V& v){
			return _datas[_hash(k) % _bucket]->get(k, v);
		}

		V get(const K& k){
			return _datas[_hash(k) % _bucket]->get(k);
		}

		bool del(const k& k){
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

		void setMaxSize(const size_t &v){
			size_t pre_max_size = std::ceil(v * 1.0 / _bucket);
			_maxSize = pre_max_size * _bucket;
			for(auto& i : _datas){
				i->setMaxSize(pre_max_size);
			}
		}

		void setElasticity(const size_t &v){
			size_t pre_ela = std::ceil(v * 1.0 / _bucket);
			_elasticity = pre_ela * _bucket;
			for(auto& i : _datas){
				i->setElasticity(pre_ela);
			}
		}

		template<class F>
		void foreach(F& f){
			for(auto &i : _datas){
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

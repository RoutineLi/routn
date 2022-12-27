/*************************************************************************
	> File Name: Cache.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月22日 星期四 23时38分01秒
 ************************************************************************/

#ifndef _CACHE_H
#define _CACHE_H

#include <sstream>
#include <cstdint>
#include <string>
#include "../Util.h"

//缓存相关
namespace Routn{
namespace Ds{

	class CacheStatus{
	public:
		CacheStatus(){}

		int64_t incGet(int64_t v = 1) { return Atomic::addFetch(_get, v);}
		int64_t incSet(int64_t v = 1) { return Atomic::addFetch(_set, v);}
		int64_t incDel(int64_t v = 1) { return Atomic::addFetch(_del, v);}
		int64_t incTimeout(int64_t v = 1) { return Atomic::addFetch(_timeout, v);}
		int64_t incPrune(int64_t v = 1) { return Atomic::addFetch(_prune, v);}
		int64_t incHit(int64_t v = 1) { return Atomic::addFetch(_hit, v);}

		int64_t decGet(int64_t v = 1) { return Atomic::subFetch(_get, v);}
		int64_t decSet(int64_t v = 1) { return Atomic::subFetch(_set, v);}
		int64_t decDel(int64_t v = 1) { return Atomic::subFetch(_del, v);}
		int64_t decTimeout(int64_t v = 1) { return Atomic::subFetch(_timeout, v);}
		int64_t decPrune(int64_t v = 1) { return Atomic::subFetch(_prune, v);}
		int64_t decHit(int64_t v = 1) { return Atomic::subFetch(_hit, v);}

		int64_t getGet() const { return _get;}
		int64_t getSet() const { return _set;}
		int64_t getDel() const { return _del;}
		int64_t getTimeout() const { return _timeout;}
		int64_t getPrune() const { return _prune;}
		int64_t getHit() const { return _hit;}

///缓存命中率
		double getHitRate() const {
			return _get ? (_hit * 1.0 / _get) : 0;
		}
///合并
		void merge(const CacheStatus& o){
			_get += o._get;
			_set += o._set;
			_del += o._del;
			_timeout += o._timeout;
			_prune += o._prune;
			_hit += o._hit;
		}

		std::string toString() const {
			std::stringstream ss;
			ss << "get=" << _get
			<< " set=" << _set
			<< " del=" << _del
			<< " prune=" << _prune
			<< " timeout=" << _timeout
			<< " hit=" << _hit
			<< " hit_rate=" << (getHitRate() * 100.0) << "%";
			return ss.str();
		}
	private:
		int64_t _get = 0;
		int64_t _set = 0;
		int64_t _del = 0;
		int64_t _timeout = 0;
		int64_t _prune = 0;
		int64_t _hit = 0;
	};


}
}


#endif

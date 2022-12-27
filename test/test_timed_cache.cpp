/*************************************************************************
	> File Name: test_timed_cache.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月23日 星期五 19时13分05秒
 ************************************************************************/

#include "src/Routn.h"
#include "src/ds/TimedCache.h"

void test1(){
	Routn::Ds::Timed_Cache<int, int> cache(30, 10);
	for(int i = 0; i < 105; ++i){
		cache.set(i, i * 100, 1000);
	}

	for(int i = 0; i < 105; ++i){
		int v;
		if(cache.get(i, v)){
			std::cout << "get: " << i << " - " << v << " - " << cache.get(i) << std::endl;
		}
	}

	cache.set(1000, 11, 1000 * 10);
	std::cout << "expired: " << cache.expired(100, 1000 * 10) << std::endl;
	std::cout << cache.toStatusString() << std::endl;
	sleep(2);
	std::cout << "check_timeout: " << cache.checkTimeout() << std::endl;
	std::cout << cache.toStatusString() << std::endl;
}

void test2(){
	Routn::Ds::HashTimed_Cache<int, int> cache(2, 30, 10);
	for(int i = 0; i < 105; ++i){
		cache.set(i, i * 100, 1000);
	}

	for(int i = 0; i < 105; ++i){
		int v;
		if(cache.get(i, v)){
			std::cout << "get: " << i << " - " << v << " - " << cache.get(i) << std::endl;
		}
	}

	cache.set(1000, 11, 1000 * 10);
	std::cout << "expired: " << cache.expired(100, 1000 * 10) << std::endl;
	std::cout << cache.toStatusString() << std::endl;
	sleep(2);
	std::cout << "check_timeout: " << cache.checkTimeout() << std::endl;
	std::cout << cache.toStatusString() << std::endl;
}

int main(int argc, char** argv){
	test1();
	test2();
}


/*************************************************************************
	> File Name: test_lru.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月23日 星期五 18时22分47秒
 ************************************************************************/

#include "src/Routn.h"
#include "src/ds/LRUCache.h"

void test1(){
	Routn::Ds::LRU_Cache<int, int> cache(30, 10);

	for(int i = 0; i < 105; ++i){
		cache.set(i, i * 100);
	}

	for(int i = 0; i < 105; ++i){
		int v;
		if(cache.get(i, v)){
			std::cout << "get: " << i << " - " << v << std::endl;
		}
	}

	std::cout << cache.toStatusString() << std::endl;
}

void test2(){
	Routn::Ds::HashLRU_Cache<int, int> cache(2, 30, 10);

	for(int i = 0; i < 105; ++i){
		cache.set(i, i * 100);
	}

	for(int i = 0; i < 105; ++i){
		int v;
		if(cache.get(i, v)){
			std::cout << "get: " << i << " - " << v << std::endl;
		}
	}

	std::cout << cache.toStatusString() << std::endl;
}

int main(int argc, char** argv){
	test1();
	test2();
	return 0;
}


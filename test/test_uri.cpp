/*************************************************************************
	> File Name: test_uri.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月24日 星期四 01时58分10秒
 ************************************************************************/

#include "../src/Uri.h"
#include <iostream>

int main(int argc, char** argv){
	Routn::Uri::ptr uri = Routn::Uri::Create("http://www.sylar.top/test/uri?id=100&name=sylar#frg你好");
	std::cout << uri->toString() << std::endl;
	auto addr = uri->createAddress();
	std::cout << *addr << std::endl;
	return 0;
}


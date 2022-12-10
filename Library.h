/*************************************************************************
	> File Name: Library.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月28日 星期一 10时41分51秒
 ************************************************************************/

#ifndef _LIBRARY_H
#define _LIBRARY_H

#include <memory>
#include "Module.h"

/**
 * Module动态库加载类
 */
namespace Routn{

	class Library{
	public:
		static Module::ptr GetModule(const std::string& path);
	};
}

#endif

/*************************************************************************
	> File Name: heart_module.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月28日 星期一 13时27分11秒
 ************************************************************************/

#ifndef _HEART_MODULE_H
#define _HEART_MODULE_H

#include "src/Module.h"
#include <memory>

using namespace Routn;

class HeartModule : public Module{
public:
	using ptr = std::shared_ptr<HeartModule>;
	HeartModule();

	bool onLoad() override;
	bool onUnload() override;
	bool onServerReady() override;
	bool onServerUp() override;
};

#endif

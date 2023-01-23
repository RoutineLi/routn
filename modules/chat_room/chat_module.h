/*************************************************************************
	> File Name: chat_module.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月16日 星期五 19时39分52秒
 ************************************************************************/

#ifndef _CHAT_MODULE_H
#define _CHAT_MODULE_H

#include "src/Module.h"
#include "chat_servlet.h"

namespace Chat{
	class ChatModule : public Routn::Module{
	public:
		using ptr = std::shared_ptr<ChatModule>;
		ChatModule();

		bool onLoad() override;
		bool onUnload() override;
		bool onServerReady() override;
		bool onServerUp() override;
	};
}

#endif

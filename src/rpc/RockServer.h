/*************************************************************************
	> File Name: RockServer.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月02日 星期一 13时10分26秒
 ************************************************************************/

#ifndef _ROCKSERVER_H
#define _ROCKSERVER_H

#include "RockStream.h"
#include "../TcpServer.h"

namespace Routn{
	class RockServer : public TcpServer{
	public:
		using ptr = std::shared_ptr<RockServer>;
		RockServer(const std::string& type = "rock"
					, IOManager* worker = IOManager::GetThis()
					, IOManager* io_worker = IOManager::GetThis()
					, IOManager* accept_worker = IOManager::GetThis());
	protected:
		virtual void handleClient(Socket::ptr client) override;
	};
}

#endif

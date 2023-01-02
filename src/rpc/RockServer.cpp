/*************************************************************************
	> File Name: RockServer.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月02日 星期一 13时10分32秒
 ************************************************************************/

#include "RockServer.h"
#include "../Log.h"
#include "../Module.h"

namespace Routn{
	
	RockServer::RockServer(const std::string& type
						, IOManager* worker
						, IOManager* io_worker
						, IOManager* accept_worker)
		: TcpServer(worker, io_worker, accept_worker){
		_type = type;
	}

	void RockServer::handleClient(Socket::ptr client){
		ROUTN_LOG_DEBUG(g_logger) << "handleClient " << *client;
		RockSession::ptr session(new Routn::RockSession(client));
		session->setWorker(_worker);
		ModuleMgr::GetInstance()->foreach(Module::ROCK, 
				[session](Module::ptr m){
			m->onConnect(session);
		});
		session->setDisConnectCB([](AsyncSocketStream::ptr stream){
			ModuleMgr::GetInstance()->foreach(Module::ROCK,
				[stream](Module::ptr m){
				m->onDisconnect(stream);
			});
		});
		session->setRequestHandler([](RockRequest::ptr req, RockResponse::ptr rsp, RockStream::ptr conn)->bool {
			bool rt = false;
			ModuleMgr::GetInstance()->foreach(Module::ROCK, [&rt, req, rsp, conn](Module::ptr m){
				if(rt){
					return ;
				}
				rt = m->handleRequest(req, rsp, conn);
			});
			return rt;
		});
		session->setNotifyHandler([](RockNotify::ptr nty, RockStream::ptr conn)->bool {
			ROUTN_LOG_INFO(g_logger) << "handleNotify " << nty->toString()
									 << " body = " << nty->getBody();
			bool rt = false;
			ModuleMgr::GetInstance()->foreach(Module::ROCK,
				[&rt, nty, conn](Module::ptr m){
					if(rt){
						return;
					}
					rt = m->handleNotify(nty, conn);
			});
			return rt;
		});
		session->start();
	}

}


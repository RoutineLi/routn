/*************************************************************************
	> File Name: WSServer.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月10日 星期六 00时41分32秒
 ************************************************************************/

#include "WSServer.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn{
namespace Http{
	WSServer::WSServer(IOManager* worker, IOManager* io_worker, IOManager* accept_worker)
		: TcpServer(worker, io_worker, accept_worker){
		_dispatch = std::make_shared<WSServletDispatch>();
		_type = "websocket_server";
	}

	void WSServer::handleClient(Socket::ptr client) {
		WSSession::ptr session = std::make_shared<WSSession>(client);
		do{
			HttpRequest::ptr header = session->handleShake();
			if(!header){
				ROUTN_LOG_DEBUG(g_logger) << "handleShake error";
				break;
			}
			WSServlet::ptr servlet = _dispatch->getWSServlet(header->getPath());
			if(!servlet){
				ROUTN_LOG_DEBUG(g_logger) << "no match WSServlet";
				break;
			}
			int ret = servlet->onConnect(header, session);
			if(ret){
				ROUTN_LOG_DEBUG(g_logger) << "onConnect return" << ret;
				break;
			}
			while(true){
				auto msg = session->recvMessage();
				if(!msg){
					session->close();
					break;
				}
				ret = servlet->handle(header, msg, session);
				if(ret){
					ROUTN_LOG_DEBUG(g_logger) << "handle return " << ret;
					break;
				}
			}
			servlet->onClose(header, session);
		}while(false);
		session->close();
	}
}
}

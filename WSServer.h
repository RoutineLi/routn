/*************************************************************************
	> File Name: WSServer.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月10日 星期六 00时41分23秒
 ************************************************************************/

#ifndef _WSSERVER_H
#define _WSSERVER_H

#include "../TcpServer.h"
#include "WSServlet.h"
#include "WSSession.h"

namespace Routn{
namespace Http{

	class WSServer : public TcpServer{
	public:
		using ptr = std::shared_ptr<WSServer>;

		WSServer(Routn::IOManager* worker = Routn::IOManager::GetThis()
				, Routn::IOManager* io_worker = Routn::IOManager::GetThis()
				, Routn::IOManager* accept_worker = Routn::IOManager::GetThis());

		WSServletDispatch::ptr getWSServletDispatch() const { return _dispatch;}
		void setWSServletDispatch(WSServletDispatch::ptr v) { _dispatch = v;}
	protected:
		virtual void handleClient(Socket::ptr client) override;
	protected:
		WSServletDispatch::ptr _dispatch;
		std::string _type;
	};

}
}

#endif

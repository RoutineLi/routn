/*************************************************************************
	> File Name: HttpServer.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月21日 星期一 10时27分21秒
 ************************************************************************/

#ifndef _HTTPSERVER_H
#define _HTTPSERVER_H

#include "../TcpServer.h"
#include "HttpSession.h"
#include "HttpServlet.h"

namespace Routn
{
	namespace Http
	{

		class HttpServer : public TcpServer
		{
		public:
			using ptr = std::shared_ptr<HttpServer>;
			HttpServer(bool keepalive = false, Routn::IOManager *worker = Routn::IOManager::GetThis(), Routn::IOManager *io_worker = Routn::IOManager::GetThis(), Routn::IOManager *accept_worker = Routn::IOManager::GetThis());

			ServletDispatch::ptr getServletDispatch() const { return _dispatch;}
			void setServletDispatch(ServletDispatch::ptr v) { _dispatch = v;}
		protected:
			virtual void handleClient(Socket::ptr client) override;

		private:
			bool _isKeepAlive;
			ServletDispatch::ptr _dispatch;
		};

	}
}

#endif

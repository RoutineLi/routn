/*************************************************************************
	> File Name: HttpServer.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月21日 星期一 10时32分58秒
 ************************************************************************/

#include "HttpServer.h"
#include "../Log.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn
{
	namespace Http
	{

		HttpServer::HttpServer(bool keepalive, Routn::IOManager *worker, Routn::IOManager *io_worker, Routn::IOManager *accept_worker)
			: TcpServer(worker, io_worker, accept_worker), _isKeepAlive(keepalive)
		{
			_dispatch.reset(new ServletDispatch);
		}

		void HttpServer::handleClient(Socket::ptr client)
		{
			Routn::Http::HttpSession::ptr session(new HttpSession(client));
			do{
				HttpRequest::ptr req = session->recvRequest();
				
				if(errno && errno != 11){
					ROUTN_LOG_WARN(g_logger) << "recv http request fail, errno = "
						<< errno << " reason = " << strerror(errno)
						<< " client: " << *client;
					break;					
				}
				
				if(!req){
					ROUTN_LOG_WARN(g_logger) << "recv http request fail, errno = "
						<< errno << " reason = " << strerror(errno)
						<< " client: " << *client;
					break;
				}


				HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), (req->isClose() || !_isKeepAlive)));
				rsp->setHeader("Server", getName());
            	_dispatch->handle(req, rsp, session);
				session->sendResponse(rsp);
				if(!_isKeepAlive || req->isClose()){
					break;
				}
			}while(true);
			session->close();
		}

	}
}

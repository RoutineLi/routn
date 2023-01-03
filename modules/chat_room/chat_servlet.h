/*************************************************************************
	> File Name: chat_servlet.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月16日 星期五 19时57分31秒
 ************************************************************************/

#ifndef _CHAT_SERVLET_H
#define _CHAT_SERVLET_H

#include "src/http/HttpServlet.h"
#include "src/http/WSServlet.h"


// TODO as template



class ChatServlet : public Routn::Http::WSServlet{
public:
	ChatServlet();
	virtual int32_t onConnect(Routn::Http::HttpRequest::ptr header
					, Routn::Http::WSSession::ptr session) override;
	virtual int32_t onClose(Routn::Http::HttpRequest::ptr header
					, Routn::Http::WSSession::ptr session) override;
	virtual int32_t handle(Routn::Http::HttpRequest::ptr header
					, Routn::Http::WSFrameMessage::ptr msg
					, Routn::Http::WSSession::ptr session) override;
};



namespace Routn{
namespace Http{
	class ResourceServlet : public Servlet{
	public:
		using ptr = std::shared_ptr<ResourceServlet>;
		ResourceServlet(const std::string& path);
		virtual int32_t handle(Routn::Http::HttpRequest::ptr request, 
							Routn::Http::HttpResponse::ptr response,
							Routn::SocketStream::ptr session) override;	

	private:
		std::string _path;
	};
}
}
#endif

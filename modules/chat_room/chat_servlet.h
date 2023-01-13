/*************************************************************************
	> File Name: chat_servlet.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月16日 星期五 19时57分31秒
 ************************************************************************/

#ifndef _CHAT_SERVLET_H
#define _CHAT_SERVLET_H

#include "src/http/HttpServlet.h"

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

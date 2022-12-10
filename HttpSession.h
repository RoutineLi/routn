/*************************************************************************
	> File Name: HttpSession.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月21日 星期一 09时43分10秒
 ************************************************************************/

#ifndef _HTTPSESSION_H
#define _HTTPSESSION_H

#include "../SocketStream.h"
#include "HTTP.h"

namespace Routn
{
	namespace Http
	{
		class HttpSession : public SocketStream
		{
		public:
			using ptr = std::shared_ptr<HttpSession>;
			HttpSession(Socket::ptr sock, bool owner = true);
			HttpRequest::ptr recvRequest();			 //处理请求
			int sendResponse(HttpResponse::ptr rsp); //发送响应
		private:
		};
	}
}

#endif

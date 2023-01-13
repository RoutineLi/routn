/*************************************************************************
	> File Name: HttpSession.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月21日 星期一 09时47分11秒
 ************************************************************************/

#include "HttpSession.h"
#include "Parser.h"
#include "../Log.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn
{
	namespace Http
	{
		HttpSession::HttpSession(Socket::ptr sock, bool owner)
			: SocketStream(sock, owner)
		{
		}

		int HttpSession::sendResponse(HttpResponse::ptr rsp)
		{
			std::stringstream ss;
			ss << *rsp;
			std::string data = ss.str();
			return writeFixSize(data.c_str(), data.size());
		}

		HttpRequest::ptr HttpSession::recvRequest()
		{
			HttpRequestParser::ptr parser(new HttpRequestParser);
			uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
			std::shared_ptr<char> buffer(new char[buff_size], [](char *ptr)
										 { delete[] ptr; });
			char *data = buffer.get();
			int offset = 0;
			do
			{
				int len = read(data + offset, buff_size - offset);
				if (len <= 0)
				{
					close();
					return nullptr;
				}
				len += offset;
				size_t nparse = parser->execute(data, len);
				if (parser->hasError())
				{
					close();
					return nullptr;
				}
				offset = len - nparse;
				if (offset == (int)buff_size)
				{
					close();
					return nullptr;
				}
				if (parser->isFinished())
				{
					break;
				}
			} while (true);
			int64_t length = parser->getContentLength();
			if (length > 0)
			{
				std::string body;
				body.resize(length);

				int len = 0;
				if (length >= offset)
				{
					memcpy(&body[0], data, offset);
					len = offset;
				}
				else
				{
					memcpy(&body[0], data, length);
					len = length;
				}
				length -= offset;
				if (length > 0)
				{
					if (readFixSize(&body[len], length) <= 0)
					{
						close();
						return nullptr;
					}
				}
				parser->getData()->setBody(body);
			}
			parser->getData()->init();
			return parser->getData();
		}

	}
}

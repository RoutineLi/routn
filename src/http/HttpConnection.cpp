/*************************************************************************
	> File Name: HttpConnection.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月21日 星期一 09时47分11秒
 ************************************************************************/

#include "HttpConnection.h"
#include "Parser.h"
#include "../Util.h"
#include "../Log.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn
{
	namespace Http
	{
		HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
			: SocketStream(sock, owner)
		{
		}

		HttpConnection::~HttpConnection()
		{
			ROUTN_LOG_DEBUG(g_logger) << "HttpConnection::~Connection";
		}

		int HttpConnection::sendRequest(HttpRequest::ptr req)
		{
			std::stringstream ss;
			ss << *req;
			std::string data = ss.str();
			return writeFixSize(data.c_str(), data.size());
		}

		HttpResponse::ptr HttpConnection::recvResponse()
		{
			HttpResponseParser::ptr parser(new HttpResponseParser);
			uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize();
			std::shared_ptr<char> buffer(new char[buff_size + 1], [](char *ptr)
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
				data[len] = '\0';
				size_t nparse = parser->execute(data, len, false);
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
			auto& client_parser = parser->getParser();
			std::string body;
			if(client_parser.chunked){
				int len = offset;
				do{
					bool begin = true;
					do{
						if(!begin || len == 0){
							int ret = read(data + len, buff_size - len);
							if(ret <= 0){
								close();
								return nullptr;
							}
							len += ret;
						}
						data[len] = '\0';
						size_t nparse = parser->execute(data, len, true);
						if(parser->hasError()){
							close();
							return nullptr;
						}
						len -= nparse;
						if(len == (int)buff_size){
							close();
							return nullptr;
						}
						begin = false;
					}while(!parser->isFinished());
					//len -= 2;

					if(client_parser.content_len + 2 <= len){
						body.append(data, client_parser.content_len);
						memmove(data, data + client_parser.content_len + 2, len - client_parser.content_len - 2);
						len -= client_parser.content_len + 2;
					}else{
						body.append(data, len);
						len = 0;
						int left = client_parser.content_len - len + 2;
						while(left > 0){
							int ret = read(data, left > (int)buff_size ? (int)buff_size : left);
							if(ret <= 0){
								return nullptr;
							}
							body.append(data, ret);
							left -= ret;
						}
						body.resize(body.size() - 2);
						len = 0;
					}

				}while(!client_parser.chunks_done);
				parser->getData()->setBody(body);
			}else{
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
			}
			return parser->getData();
		}

		HttpResult::ptr HttpConnection::DoGet(const std::string& url
										, uint64_t timeout_ms
										, const std::map<std::string, std::string>& headers
										, const std::string& body)
		{
			Uri::ptr uri = Uri::Create(url);
			if(!uri){
				return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
			}
			return DoGet_fun(uri, timeout_ms, headers, body);
		} 

		HttpResult::ptr HttpConnection::DoGet_fun(Uri::ptr uri
										, uint64_t timeout_ms
										, const std::map<std::string, std::string>& headers
										, const std::string& body)
		{
			return DoRequest_fun(HttpMethod::GET, uri, timeout_ms, headers, body);
		}

		HttpResult::ptr HttpConnection::DoPost_fun(Uri::ptr uri
										, uint64_t timeout_ms
										, const std::map<std::string, std::string>& headers
										, const std::string& body)
		{
			return DoRequest_fun(HttpMethod::POST, uri, timeout_ms, headers, body);
		}

		HttpResult::ptr HttpConnection::DoPost(const std::string& url
										, uint64_t timeout_ms
										, const std::map<std::string, std::string>& headers
										, const std::string& body)
		{
			Uri::ptr uri = Uri::Create(url);
			if(!uri){
				return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
			}
			return DoPost_fun(uri, timeout_ms, headers, body);
		}

		HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
										, const std::string& url
										, uint64_t timeout_ms
										, const std::map<std::string, std::string>& headers
										, const std::string& body) 
		{
			Uri::ptr uri = Uri::Create(url);
			if(!uri){
				return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
			}
			return DoRequest_fun(method, uri, timeout_ms, headers, body);
		}

		HttpResult::ptr HttpConnection::DoRequest_fun(HttpMethod method
										, Uri::ptr uri
										, uint64_t timeout_ms
										, const std::map<std::string, std::string>& headers
										, const std::string& body)
		{
			HttpRequest::ptr req(new HttpRequest);
			req->setPath(uri->getPath());
			req->setQuery(uri->getQuery());
			req->setFragment(uri->getFragment());
			req->setMethod(method);
			req->setClose(false);
			bool has_host = false;
			for(auto& i : headers){
				if(strcasecmp(i.first.c_str(), "connection") == 0){
					if(strcasecmp(i.second.c_str(), "keep-alive") == 0){
						req->setClose(false);
					}
					continue;
				}

				if(!has_host && strcasecmp(i.first.c_str(), "host") == 0){
					has_host = !i.second.empty();
				}

				req->setHeader(i.first, i.second);
			}
			
			if(!has_host){
				req->setHeader("Host", uri->getHost());
			}
			req->setBody(body);

			return DoRequest_base(req, uri, timeout_ms);
		}

		HttpResult::ptr HttpConnection::DoRequest_base(HttpRequest::ptr req
										, Uri::ptr uri
										, uint64_t timeout_ms)  
		{
			Address::ptr addr = uri->createAddress();
			if(!addr){
				return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST, nullptr, "invalid host: " + uri->getHost());
			}
			Socket::ptr sock = Socket::CreateTCP(addr);
			if(!sock){
				return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL, nullptr, "connected fail: " + addr->toString());
			}
			sock->setRecvTimeout(timeout_ms);
			auto conn = std::make_shared<HttpConnection>(sock);
			
			if(!sock->connect(addr)){
				return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL, nullptr, "connect fail: " + addr->toString());
			}

			int ret = conn->sendRequest(req);
			if(ret == 0){
				return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_REMOTE, nullptr, "send request closed by remote: " + addr->toString());
			}else if(ret < 0){
				return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr, "send socket error, errno =  " + std::to_string(errno) + " reason: " + std::string(strerror(errno)));
			}
			auto rsp = conn->recvResponse();
			if(!rsp){
				//TODO
				return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr, "recv response timeout: " + addr->toString() + " timout_ms = " + std::to_string(timeout_ms));
			}
			return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
		}	


		HttpConnectionPool::HttpConnectionPool(const std::string& host
								, const std::string& vhost
								, uint32_t port
								, uint32_t max_size
								, uint32_t alive_time
								, uint32_t max_request)
			: _host(host)
			, _vhost(vhost)
			, _port(port)
			, _maxSize(max_size)
			, _maxAliveTime(alive_time)
			, _maxRequest(max_request) {	
		}

		HttpConnection::ptr HttpConnectionPool::getConnection(){
			uint64_t now_ms = Routn::GetCurrentMs();
			std::vector<HttpConnection*> invalid_conns;
			HttpConnection* ptr = nullptr;
			MutexType::Lock lock(_mutex);
			while(!_conns.empty()){
				auto conn = *_conns.begin();
				_conns.pop_front();
				if(!conn->isConnected()){
					invalid_conns.push_back(conn);
					continue;
				}
				if((conn->_create_time + _maxAliveTime) > now_ms){
					invalid_conns.push_back(conn);
					continue;
				}
				ptr = conn;
				break;
			}
			lock.unlock();
			for(auto i : invalid_conns){
				delete i;
			}
			_total -= invalid_conns.size();

			if(!ptr){
				IPAddress::ptr addr = Address::LookupAnyIPAddress(_host);
				if(!addr){
					ROUTN_LOG_ERROR(g_logger) << "get addr fail: " << addr->toString();
					return nullptr;
				}
				addr->setPort(_port);
				Socket::ptr sock = Socket::CreateTCP(addr);
				if(!sock){
					ROUTN_LOG_ERROR(g_logger) << "create socket fail: " << *addr;
					return nullptr;
				}
				if(!sock->connect(addr)){
					ROUTN_LOG_ERROR(g_logger) << "socket connect fail: " << *addr;
					return nullptr;
				}
				ptr = new HttpConnection(sock);
				++_total;
			}
			return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr, std::placeholders::_1, this));
		}

		HttpResult::ptr HttpConnectionPool::doGet(const std::string& url
										, uint64_t timeout_ms
										, const std::map<std::string, std::string>& headers
										, const std::string& body){
			return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
		}

		HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri
										, uint64_t timeout_ms
										, const std::map<std::string, std::string>& headers
										, const std::string& body){
			std::stringstream ss;
			ss << uri->getPath()
			   << (uri->getQuery().empty() ? "" : "?")
			   << uri->getQuery()
			   << (uri->getFragment().empty() ? "" : "#")
			   << uri->getFragment();
			return doGet(ss.str(), timeout_ms, headers, body);

		}

		HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri
										, uint64_t timeout_ms
										, const std::map<std::string, std::string>& headers
										, const std::string& body){
			std::stringstream ss;
			ss << uri->getPath()
			   << (uri->getQuery().empty() ? "" : "?")
			   << uri->getQuery()
			   << (uri->getFragment().empty() ? "" : "#")
			   << uri->getFragment();
			return doPost(ss.str(), timeout_ms, headers, body);
		}

		HttpResult::ptr HttpConnectionPool::doPost(const std::string& url
										, uint64_t timeout_ms
										, const std::map<std::string, std::string>& headers
										, const std::string& body){
			return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);									
		}

		HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
										, const std::string& url
										, uint64_t timeout_ms
										, const std::map<std::string, std::string>& headers 
										, const std::string& body){
			HttpRequest::ptr req(new HttpRequest);
			req->setPath(url);
			req->setMethod(method);
			bool has_host = false;
			for(auto& i : headers){
				if(strcasecmp(i.first.c_str(), "connection") == 0){
					if(strcasecmp(i.second.c_str(), "keep-alive") == 0){
						req->setClose(false);
					}
					continue;
				}

				if(!has_host && strcasecmp(i.first.c_str(), "host") == 0){
					has_host = !i.second.empty();
				}

				req->setHeader(i.first, i.second);
			}
			
			if(!has_host){
				if(_vhost.empty()){
					req->setHeader("Host", _host);
				}else{
					req->setHeader("Host", _vhost);
				}
			}
			req->setBody(body);

			return doRequest(req, timeout_ms);
		} 
		
		HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
										, Uri::ptr uri
										, uint64_t timeout_ms
										, const std::map<std::string, std::string>& headers 
										, const std::string& body){
			std::stringstream ss;
			ss << uri->getPath()
			   << (uri->getQuery().empty() ? "" : "?")
			   << uri->getQuery()
			   << (uri->getFragment().empty() ? "" : "#")
			   << uri->getFragment();
			return doRequest(method, ss.str(), timeout_ms, headers, body);
		}
		
		HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req
										, uint64_t timeout_ms){
			auto conn = getConnection();
			if(!conn){
				return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION, nullptr, "pool host: " + _host + " port: " + std::to_string(_port));
			}
			auto sock = conn->getSocket();
			if(!sock){
				return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL, nullptr, "connected fail: " );
			}
			sock->setRecvTimeout(timeout_ms);
			int ret = conn->sendRequest(req);
			if(ret == 0) {
				return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_REMOTE
						, nullptr, "send request closed by peer: " + sock->getRemoteAddress()->toString());
			}
			if(ret < 0) {
				return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
							, nullptr, "send request socket error errno=" + std::to_string(errno)
							+ " errstr=" + std::string(strerror(errno)));
			}
			auto rsp = conn->recvResponse();
			if(!rsp) {
				return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
							, nullptr, "recv response timeout: " + sock->getRemoteAddress()->toString()
							+ " timeout_ms:" + std::to_string(timeout_ms));
			}
			return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
		}  

		void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool){
			++ptr->_request_count;
			if(!ptr->isConnected()
				|| (ptr->_create_time + pool->_maxAliveTime) >= Routn::GetCurrentMs()
				|| (ptr->_request_count >= pool->_maxRequest)){
				delete ptr;
				--pool->_total;
				return ;
			}
			MutexType::Lock lock(pool->_mutex);
			pool->_conns.push_back(ptr);
			
		}
		
	}
}

/*************************************************************************
	> File Name: HttpSession.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月21日 星期一 09时43分10秒
 ************************************************************************/

#ifndef _HTTPCONNECTION_H
#define _HTTPCONNECTION_H

#include <atomic>
#include <list>

#include "HTTP.h"
#include "../SocketStream.h"
#include "../Thread.h"
#include "../Uri.h"

namespace Routn
{
	namespace Http
	{
		struct HttpResult{
			using ptr = std::shared_ptr<HttpResult>;
			enum class Error{
				OK 						= 0,
				INVALID_URL 			= 1,
				INVALID_HOST 			= 2,
				CONNECT_FAIL 			= 3,
				SEND_CLOSE_BY_REMOTE 	= 4,
				SEND_SOCKET_ERROR 		= 5,
				TIMEOUT 				= 6,
				CREATE_SOCKET_ERROR 	= 7,
				POOL_GET_CONNECTION 	= 8
			};
			HttpResult(int _result 
						, HttpResponse::ptr _response
						, const std::string& _error)
				: result(_result)
				, response(_response)
				, error(_error){
			}
			int result;
			HttpResponse::ptr response;
			std::string error;

			std::string toString() const {
				std::stringstream ss;
				ss << "{HttpResult [result] = " << result
				   << " [error] = " << error
				   << " [response] = " << (response ? response->toString() : "Null")
				   << "}";
				return ss.str();
			}
		};

		class HttpConnectionPool;

		class HttpConnection : public SocketStream
		{
			friend class HttpConnectionPool;
		public:
			using ptr = std::shared_ptr<HttpConnection>;
			static HttpResult::ptr DoGet(const std::string& url
											, uint64_t timeout_ms
											, const std::map<std::string, std::string>& headers = {}
											, const std::string& body = ""); 

			static HttpResult::ptr DoGet_fun(Uri::ptr uri
											, uint64_t timeout_ms
											, const std::map<std::string, std::string>& headers = {}
											, const std::string& body = "");

			static HttpResult::ptr DoPost_fun(Uri::ptr uri
											, uint64_t timeout_ms
											, const std::map<std::string, std::string>& headers = {}
											, const std::string& body = ""); 

			static HttpResult::ptr DoPost(const std::string& url
											, uint64_t timeout_ms
											, const std::map<std::string, std::string>& headers = {}
											, const std::string& body = ""); 

			static HttpResult::ptr DoRequest(HttpMethod method
											, const std::string& url
											, uint64_t timeout_ms
											, const std::map<std::string, std::string>& headers = {}
											, const std::string& body = ""); 
			
			static HttpResult::ptr DoRequest_fun(HttpMethod method
											, Uri::ptr uri
											, uint64_t timeout_ms
											, const std::map<std::string, std::string>& headers = {}
											, const std::string& body = "");
			
			static HttpResult::ptr DoRequest_base(HttpRequest::ptr req
											, Uri::ptr uri
											, uint64_t timeout_ms);  
			
			
			HttpConnection(Socket::ptr sock, bool owner = true);
			~HttpConnection();
			HttpResponse::ptr recvResponse();			 //处理请求
			int sendRequest(HttpRequest::ptr req); 		 //发送响应
		private:
			uint64_t _create_time = 0;
			uint64_t _request_count = 0;
		};

		class HttpConnectionPool{
		public:
			using ptr = std::shared_ptr<HttpConnectionPool> ;
			using MutexType = Mutex;

			HttpConnectionPool(const std::string& host
								, const std::string& vhost
								, uint32_t port
								, uint32_t max_size
								, uint32_t alive_time
								, uint32_t max_request);

			HttpConnection::ptr getConnection();

			HttpResult::ptr doGet(const std::string& url
											, uint64_t timeout_ms
											, const std::map<std::string, std::string>& headers = {}
											, const std::string& body = ""); 

			HttpResult::ptr doGet(Uri::ptr uri
											, uint64_t timeout_ms
											, const std::map<std::string, std::string>& headers = {}
											, const std::string& body = "");

			HttpResult::ptr doPost(Uri::ptr uri
											, uint64_t timeout_ms
											, const std::map<std::string, std::string>& headers = {}
											, const std::string& body = ""); 

			HttpResult::ptr doPost(const std::string& url
											, uint64_t timeout_ms
											, const std::map<std::string, std::string>& headers = {}
											, const std::string& body = ""); 

			HttpResult::ptr doRequest(HttpMethod method
											, const std::string& url
											, uint64_t timeout_ms
											, const std::map<std::string, std::string>& headers = {}
											, const std::string& body = ""); 
			
			HttpResult::ptr doRequest(HttpMethod method
											, Uri::ptr uri
											, uint64_t timeout_ms
											, const std::map<std::string, std::string>& headers = {}
											, const std::string& body = "");
			
			HttpResult::ptr doRequest(HttpRequest::ptr req
											, uint64_t timeout_ms);  

		private:	  
			static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);
		private:
			std::string _host;
			std::string _vhost;
			uint32_t _port;
			uint32_t _maxSize;
			uint32_t _maxAliveTime;
			uint32_t _maxRequest;

			MutexType _mutex;
			std::list<HttpConnection*> _conns;
			std::atomic<int32_t> _total = {0};
		};
	}
}

#endif

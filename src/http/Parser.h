/*************************************************************************
	> File Name: Parser.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月18日 星期五 14时54分15秒
 ************************************************************************/

#ifndef _PARSER_H
#define _PARSER_H

#include <string.h>

#include "HTTP.h"

namespace Routn
{
	namespace Http
	{		
		static std::ostream& operator<<(std::ostream& os, HttpRequest& req){
			return req.dump(os);
		}

		static std::ostream& operator<<(std::ostream& os, HttpResponse& rsp){
			return rsp.dump(os);
		}
		class HttpRequestParser
		{
		public:
			using ptr = std::shared_ptr<HttpRequestParser>;
			HttpRequestParser();

			size_t execute(char *data, size_t len);
			int isFinished();
			int hasError();

			HttpRequest::ptr getData() const { return _data;}
			void setError(int v) { _error = v;}

			uint64_t getContentLength();
			const http_parser& getParser() const { return _parser;}
		public:
			static uint64_t GetHttpRequestBufferSize();
			static uint64_t GetHttpRequestBodySize();
		private:
			http_parser _parser;
			HttpRequest::ptr _data;
			int _error = 0;					/* 1000: invalid method;	1001: invalid version;	1002: invalid field*/
		};

		class HttpResponseParser
		{
		public:
			using ptr = std::shared_ptr<HttpResponseParser>;
			HttpResponseParser();

			size_t execute(char *data, size_t len, bool chunk);
			int isFinished();
			int hasError();

			HttpResponse::ptr getData() const { return _data;}
			void setError(int v) { _error = v;}

			uint64_t getContentLength();
			const httpclient_parser& getParser() const { return _parser;}
		public:
			static uint64_t GetHttpResponseBufferSize();
			static uint64_t GetHttpResponseBodySize();
		private:
			httpclient_parser _parser;
			HttpResponse::ptr _data;
			int _error;
		};

	}
}

#endif

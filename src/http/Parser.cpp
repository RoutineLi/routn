/*************************************************************************
	> File Name: Parser.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月18日 星期五 14时55分28秒
 ************************************************************************/

#include "Parser.h"
#include "../Log.h"
#include "../Config.h"

namespace Routn
{
	namespace Http
	{

		static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

		static Routn::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
			Routn::Config::Lookup("http.request.buffer_size", (uint64_t)(4 * 1024), "http request buffer size");
		static Routn::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
			Routn::Config::Lookup("http.request.max_body_size", (uint64_t)(64 * 1024 * 1024), "http request max_body size");
		static Routn::ConfigVar<uint64_t>::ptr g_http_response_buffer_size =
			Routn::Config::Lookup("http.response.buffer_size", (uint64_t)(4 * 1024), "http response buffer size");
		static Routn::ConfigVar<uint64_t>::ptr g_http_response_max_body_size =
			Routn::Config::Lookup("http.response.max_body_size", (uint64_t)(64 * 1024 * 1024), "http response max_body size");

		static uint64_t s_http_request_buffer_size = 0;
		static uint64_t s_http_request_max_body_size = 0;
		static uint64_t s_http_response_buffer_size = 0;
		static uint64_t s_http_response_max_body_size = 0;

	namespace{
		struct _RequestSizeIniter
		{
			_RequestSizeIniter()
			{
				s_http_request_buffer_size = g_http_request_buffer_size->getValue();
				s_http_request_max_body_size = g_http_request_max_body_size->getValue();
				s_http_response_buffer_size = g_http_response_buffer_size->getValue();
				s_http_response_max_body_size = g_http_response_max_body_size->getValue();


				g_http_request_buffer_size->addListener([](const uint64_t &ov, const uint64_t &nv)
														{ s_http_request_buffer_size = nv; });

				g_http_request_max_body_size->addListener([](const uint64_t &ov, const uint64_t &nv)
														  { s_http_request_max_body_size = nv; });

				g_http_response_buffer_size->addListener([](const uint64_t &ov, const uint64_t &nv)
														  { s_http_response_buffer_size = nv; });

				g_http_response_max_body_size->addListener([](const uint64_t &ov, const uint64_t &nv)
														  { s_http_response_max_body_size = nv; });
			}
		};
		static _RequestSizeIniter _init;
	}
		void on_request_method(void *data, const char *at, size_t length)
		{
			HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
			HttpMethod m = CharsToHttpMethod(at);

			if (m == HttpMethod::INVALID_METHOD)
			{
				ROUTN_LOG_WARN(g_logger) << "invalid http request method "
										 << std::string(at, length);
				parser->setError(1000);
				return;
			}
			parser->getData()->setMethod(m);
		}
		
		void on_request_uri(void *data, const char *at, size_t length)
		{
		}

		void on_request_fragment(void *data, const char *at, size_t length)
		{
			HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
			parser->getData()->setFragment(std::string(at, length));
		}

		void on_request_path(void *data, const char *at, size_t length)
		{
			HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
			parser->getData()->setPath(std::string(at, length));
		}
		void on_request_query(void *data, const char *at, size_t length)
		{
			HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
			parser->getData()->setQuery(std::string(at, length));
		}
		void on_request_version(void *data, const char *at, size_t length)
		{
			HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
			uint8_t v = 0;
			if (strncmp(at, "HTTP/1.1", length) == 0)
			{
				v = 0x11;
			}
			else if (strncmp(at, "HTTP/1.0", length) == 0)
			{
				v = 0x10;
			}
			else
			{
				ROUTN_LOG_WARN(g_logger) << "invalid http request version: "
										 << std::string(at, length);
				parser->setError(1001);
				return;
			}
			parser->getData()->setVersion(v);
		}
		
		void on_request_header_done(void *data, const char *at, size_t length)
		{
			
		}

		void on_request_http_field(void *data, const char *field, size_t flen,
								   const char *value, size_t vlen)
		{
			HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
			if (flen == 0)
			{
				ROUTN_LOG_WARN(g_logger) << "invalid http request field length == 0";
				//parser->setError(1002);
				return;
			}
			parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
		}

		HttpRequestParser::HttpRequestParser()
		{
			_data.reset(new Routn::Http::HttpRequest);
			http_parser_init(&_parser);
			_parser.request_method = on_request_method;
			_parser.request_path = on_request_path;
			_parser.request_uri = on_request_uri;
			_parser.fragment = on_request_fragment;
			_parser.query_string = on_request_query;
			_parser.http_version = on_request_version;
			_parser.http_field = on_request_http_field;
			_parser.header_done = on_request_header_done;
			_parser.data = this;
		}

		size_t HttpRequestParser::execute(char *data, size_t len)
		{
			size_t ret = http_parser_execute(&_parser, data, len, 0);
			memmove(data, data + ret, (len - ret));
			return ret;
		}

		int HttpRequestParser::isFinished()
		{
			return http_parser_finish(&_parser);
		}

		int HttpRequestParser::hasError()
		{
			return _error || http_parser_has_error(&_parser);
		}

		uint64_t HttpRequestParser::getContentLength(){
			return _data->getHeaderAs<uint64_t>("content-length", 0);
		}

		uint64_t HttpRequestParser::GetHttpRequestBufferSize(){
			return s_http_request_buffer_size;
		}

		uint64_t HttpRequestParser::GetHttpRequestBodySize(){
			return s_http_request_max_body_size;
		}

		uint64_t HttpResponseParser::GetHttpResponseBufferSize(){
			return s_http_request_buffer_size;
		}

		uint64_t HttpResponseParser::GetHttpResponseBodySize(){
			return s_http_request_max_body_size;
		}




		void on_response_reason(void *data, const char *at, size_t length)
		{
			HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
			parser->getData()->setReason(std::string(at, length));
		}
		void on_response_status(void *data, const char *at, size_t length)
		{
			HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
			HttpStatus status = (HttpStatus)(atoi(at));
			parser->getData()->setStatus(status);
		}
		void on_response_chunk(void *data, const char *at, size_t length)
		{	

		}
		void on_response_version(void *data, const char *at, size_t length)
		{
			HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
			uint8_t v = 0;
			if(strncmp(at, "HTTP/1.1", length) == 0){
				v = 0x11;
			}else if(strncmp(at, "HTTP/1.0", length) == 0){
				v = 0x10;
			}else {
				ROUTN_LOG_WARN(g_logger) << "invalid http response version: "
					<< std::string(at, length);
				parser->setError(1001);
				return ;
			}
			parser->getData()->setVersion(v);
		}
		void on_response_header_done(void *data, const char *at, size_t length)
		{

		}
		void on_response_last_chunk(void *data, const char *at, size_t length)
		{

		}
		void on_response_http_field(void *data, const char *field, size_t flen,
									const char *value, size_t vlen)
		{
			HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
			if(flen == 0){
				ROUTN_LOG_WARN(g_logger) << "invalid http response field length == 0";
				//parser->setError(1002);
				return ;
			}
			parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
		}

		HttpResponseParser::HttpResponseParser()
			: _error(0) {
			_data.reset(new Routn::Http::HttpResponse);
			httpclient_parser_init(&_parser);
			_parser.reason_phrase = on_response_reason;
			_parser.status_code = on_response_status;
			_parser.chunk_size = on_response_chunk;
			_parser.http_version = on_response_version;
			_parser.header_done = on_response_header_done;
			_parser.last_chunk = on_response_last_chunk;
			_parser.http_field = on_response_http_field;
			_parser.data = this;
		}
		size_t HttpResponseParser::execute(char *data, size_t len, bool chunk)
		{    
			if(chunk) {
        		httpclient_parser_init(&_parser);
    		}
			size_t ret = httpclient_parser_execute(&_parser, data, len, 0);
			memmove(data, data + ret, (len - ret));
			return ret;
		}
		int HttpResponseParser::isFinished()
		{
			return httpclient_parser_finish(&_parser);
		}
		int HttpResponseParser::hasError()
		{
			return _error || httpclient_parser_has_error(&_parser);
		}

		uint64_t HttpResponseParser::getContentLength(){
			return _data->getHeaderAs<uint64_t>("content-length", 0);
		}
	}
}

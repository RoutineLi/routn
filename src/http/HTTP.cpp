/*************************************************************************
	> File Name: HTTP.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月08日 星期二 15时07分26秒
 ************************************************************************/

#include "HTTP.h"
#include "../Util.h"

namespace Routn{
namespace Http{
	HttpMethod StringToHttpMethod(const std::string& m){
	#define XX(num, name, string) \
	if(strcmp(#string, m.c_str()) == 0){	\
		return HttpMethod::name;	\
	}
	HTTP_METHOD_MAP(XX);
	#undef XX
		return HttpMethod::INVALID_METHOD;
	}

	HttpMethod CharsToHttpMethod(const char* m){
	#define XX(num, name, string) \
	if(strncmp(#string, m, strlen(#string)) == 0){	\
		return HttpMethod::name;	\
	}
	HTTP_METHOD_MAP(XX);
	#undef XX
		return HttpMethod::INVALID_METHOD;
	}

	static const char* s_method_string[] = {
	#define XX(num, name, string) #string,
		HTTP_METHOD_MAP(XX)
	#undef XX
	};

	const char* HttpMethodToString(const HttpMethod& m){
		uint32_t ind = (uint32_t)m;
		if(ind >= sizeof(s_method_string) / sizeof(s_method_string[0])){
			return "<unknown>";
		}
		return s_method_string[ind];
	}

	const char* HttpStatusToString(const HttpStatus& s){
		switch (s)
		{
	#define XX(code, name, msg) \
		case HttpStatus::name:	\
			return #msg;		
		HTTP_STATUS_MAP(XX);
	#undef XX
		default:
			return "<unknown-status>";
		}
	}

	bool CaseInsensitiveLess::operator()(const std::string& lhs, const std::string& rhs) const{
		return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
	}
	HttpRequest::HttpRequest(uint8_t version, bool close)
		: _method(HttpMethod::GET)
		, _version(version)
		, _close(close)
		, _path("/")
		, _parserParamFlag(0){
	}

	void HttpRequest::init(){
		std::string conn = getHeader("connection");
		if(!conn.empty()){
			if(strcasecmp(conn.c_str(), "keep-alive") == 0){
				_close = false;
			}else{
				_close = true;
			}
		}
	}

	std::string HttpRequest::getHeader(const std::string& key, const std::string& def) const {
		auto it = _headers.find(key);
		return it == _headers.end() ? def : it->second;
	}

	std::string HttpRequest::getParam(const std::string& key, const std::string& def){
		initQueryParam();
		initBodyParam();
		auto it = _params.find(key);
		return it == _params.end() ? def : it->second;
	}

	std::string HttpRequest::getCookie(const std::string& key, const std::string& def){
		initCookies();
		auto it = _cookies.find(key);
		return it == _cookies.end() ? def : it->second;
	}


	void HttpRequest::setHeader(const std::string& key, const std::string& val){
		_headers[key] = val;
	}

	void HttpRequest::setParam(const std::string& key, const std::string& val){
		_params[key] = val;
	}

	void HttpRequest::setCookie(const std::string &key, const std::string &val){
		_cookies[key] = val;
	}


	void HttpRequest::delHeader(const std::string& key){
		_headers.erase(key);
	}

	void HttpRequest::delParam(const std::string& key){
		_params.erase(key);
	}

	void HttpRequest::delCookie(const std::string& key){
		_cookies.erase(key);
	}


	bool HttpRequest::hasHeader(const std::string& key, std::string* val){
		auto it = _headers.find(key);
		if(it == _headers.end()){
			return false;
		}
		if(val){
			*val = it->second;
		}
		return true;
	}

	bool HttpRequest::hasParam(const std::string& key, std::string* val){
		initQueryParam();
		initBodyParam();
		auto it = _params.find(key);
		if(it == _params.end()){
			return false;
		}
		if(val){
			*val = it->second;
		}
		return true;
	}

	bool HttpRequest::hasCookie(const std::string& key, std::string* val){
		initCookies();
		auto it = _cookies.find(key);
		if(it == _cookies.end()){
			return false;
		}
		if(val){
			*val = it->second;
		}
		return true;
	}

	void HttpRequest::initParam(){
		initQueryParam();
		initBodyParam();
		initCookies();
	}

	void HttpRequest::initQueryParam(){
		if(_parserParamFlag & 0x1){
			return ;
		}
	#define PARSE_PARAM(str, m, flag, trim) \
		size_t pos = 0; \
		do{	\
			size_t last = pos;	\
			pos = str.find('=', pos);	\
			if(pos == std::string::npos){	\
				break;	\
			}	\
			size_t key = pos;	\
			pos = str.find(flag, pos);	\
			m.insert({trim(str.substr(last, key - last)),	\
			Routn::UrlDecode(str.substr(key + 1, pos - key - 1))});	\
			if(pos == std::string::npos){	\
				break;	\
			}	\
			++pos;	\
		}while(true);

		PARSE_PARAM(_query, _params, '&',);
		_parserParamFlag |= 0x1;
	}

	void HttpRequest::initBodyParam(){
		if(_parserParamFlag & 0x2){
			return ;
		}
		std::string content_type = getHeader("content-type");
		if(strcasestr(content_type.c_str(), "application/x-www-form-urlencoded") == nullptr){
			_parserParamFlag |= 0x2;
			return ;
		}
		PARSE_PARAM(_body, _params, '&',);
		_parserParamFlag |= 0x2;
	}

	void HttpRequest::initCookies(){
		if(_parserParamFlag & 0x4){
			return ;
		}
		std::string cookie = getHeader("cookie");
		if(cookie.empty()){
			_parserParamFlag |= 0x4;
			return ;
		}
		PARSE_PARAM(cookie, _cookies, ';', Routn::Trim);
		_parserParamFlag |= 0x4;
	}
#undef PARSE_PARAM

	std::ostream& HttpRequest::dump(std::ostream& os){
		//GET /uri HTTP/1.1
		//Host: www.baidu.com
		//
		//
		os << HttpMethodToString(_method) << " "
		   << _path
		   << (_query.empty() ? "" : "?")
		   << _query
		   << (_fragment.empty() ? "" : "#")
		   << _fragment
		   << " HTTP/"
		   << ((uint32_t)(_version >> 4))
		   << "."
		   << ((uint32_t)(_version & 0x0f))
		   << "\r\n";
		for(auto &i : _headers){
			//if(strcasecmp(i.first.c_str(), "connection") == 0){
			//	continue;
			//}
			os << i.first << ":" << i.second << "\r\n";
		}
		
		if(!_body.empty()){
			os << "content-length: " << _body.size() << "\r\n\r\n"
			   << _body;
		}else{
			os << "\r\n";
		}
		return os;
	}

	std::string HttpRequest::toString() {
		std::stringstream ss;
		dump(ss);
		return ss.str();
	}

	HttpResponse::HttpResponse(uint8_t version, bool close)
		: _status(HttpStatus::OK) 
		, _version(version)
		, _close(close){
	}
	
	std::string HttpResponse::getHeader(const std::string& key, const std::string& def) const{
		auto it = _headers.find(key);
		return it == _headers.end() ? def : it->second;
	} 

	void HttpResponse::setHeader(const std::string& key, const std::string& val){
		_headers[key] = val;
	}

	void HttpResponse::delHeader(const std::string& key){
		_headers.erase(key);
	}

	void HttpResponse::setCookie(const std::string& key, const std::string& val
						, time_t expired, const std::string& path
                   		, const std::string& domain, bool secure){
	    std::stringstream ss;
		ss << key << "=" << val;
		if(expired > 0) {
			ss << ";expires=" << Routn::TimerToString(expired, "%a, %d %b %Y %H:%M:%S") << " GMT";
		}
		if(!domain.empty()) {
			ss << ";domain=" << domain;
		}
		if(!path.empty()) {
			ss << ";path=" << path;
		}
		if(secure) {
			ss << ";secure";
		}
		_cookies.push_back(ss.str());	
	}

	std::ostream& HttpResponse::dump(std::ostream& os){
		//HTTP/1.0 200 OK
		//Pragma: no-cache
		//Content-Type: text/html
		//Content-Length: 14988
		//Connection: keep-alive
		os << "HTTP/"
		   << ((uint32_t)(_version >> 4))
		   << "."
		   << ((uint32_t)(_version & 0x0f))
		   << " "
		   << (uint32_t)_status
		   << " "
		   << (_reason.empty() ? HttpStatusToString(_status) : _reason)
		   << "\r\n";
		
		for(auto &i : _headers){
			os << i.first << ": " << i.second << "\r\n";
		}

		if(!_body.empty()){
			os << "content-length: " << _body.size() << "\r\n\r\n" << _body;
		}else{
			os << "\r\n";
		}
		return os;
	}


	std::string HttpResponse::toString() {
		std::stringstream ss;
		dump(ss);
		return ss.str();
	}

}
}

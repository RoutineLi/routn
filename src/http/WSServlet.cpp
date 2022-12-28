/*************************************************************************
	> File Name: WSServlet.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月10日 星期六 00时43分16秒
 ************************************************************************/

#include "WSServlet.h"

namespace Routn{
namespace Http{

	FunctionWSServlet::FunctionWSServlet(callback cb
					, on_connect_cb connect_cb
					, on_close_cb close_cb)
		: WSServlet("Function-WebSocket-Servlet") 
		, _callback(cb)
		, _onConnect(connect_cb)
		, _onClose(close_cb) {
	}

	int32_t FunctionWSServlet::onConnect(Routn::Http::HttpRequest::ptr header
					, Routn::Http::WSSession::ptr session){
		if(_onConnect){
			return _onConnect(header, session);
		}
		return 0;
	}
	int32_t FunctionWSServlet::onClose(Routn::Http::HttpRequest::ptr header
					, Routn::Http::WSSession::ptr session){
		if(_onClose){
			return _onClose(header, session);
		}
		return 0;
	}
	int32_t FunctionWSServlet::handle(Routn::Http::HttpRequest::ptr header
					, Routn::Http::WSFrameMessage::ptr msg
					, Routn::Http::WSSession::ptr session){
		if(_callback){
			return _callback(header, msg, session);
		}
		return 0;
	}
	
	void WSServletDispatch::addServlet(const std::string& uri
						,FunctionWSServlet::callback cb
						,FunctionWSServlet::on_connect_cb connect_cb
						,FunctionWSServlet::on_close_cb close_cb) {
		ServletDispatch::addServlet(uri, std::make_shared<FunctionWSServlet>(cb, connect_cb, close_cb));
	}

	void WSServletDispatch::addServlet(const std::string& uri, WSServlet::ptr slt){
		ServletDispatch::addServlet(uri, slt);
	}

	void WSServletDispatch::addGlobServlet(const std::string& uri
						,FunctionWSServlet::callback cb
						,FunctionWSServlet::on_connect_cb connect_cb
						,FunctionWSServlet::on_close_cb close_cb) {
		ServletDispatch::addGlobServlet(uri, std::make_shared<FunctionWSServlet>(cb, connect_cb, close_cb));
	}

	WSServlet::ptr WSServletDispatch::getWSServlet(const std::string& uri) {
		auto slt = getMatchedServlet(uri);
		return std::dynamic_pointer_cast<WSServlet>(slt);
	}
}
}
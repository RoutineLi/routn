/*************************************************************************
	> File Name: WSServlet.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月10日 星期六 00时43分11秒
 ************************************************************************/

#ifndef _WSSERVLET_H
#define _WSSERVLET_H

#include "WSSession.h"
#include "../Thread.h"
#include "HttpServlet.h"

namespace Routn{
namespace Http{

	class WSServlet : public Servlet{
	public:
		using ptr = std::shared_ptr<WSServlet>;
		WSServlet(const std::string& name)
			: Servlet(name){

		}
		virtual ~WSServlet(){}

		virtual int32_t handle(Routn::Http::HttpRequest::ptr request
                   , Routn::Http::HttpResponse::ptr response
                   , Routn::SocketStream::ptr session) override{
			return 0;
		}

		virtual int32_t onConnect(Routn::Http::HttpRequest::ptr header
                		, Routn::Http::WSSession::ptr session) = 0;
		virtual int32_t onClose(Routn::Http::HttpRequest::ptr header
						, Routn::Http::WSSession::ptr session) = 0;
		virtual int32_t handle(Routn::Http::HttpRequest::ptr header
						, Routn::Http::WSFrameMessage::ptr msg
						, Routn::Http::WSSession::ptr session) = 0;
		const std::string& getName() const { return _name;}
	private:
		std::string _name;
	};


	class FunctionWSServlet : public WSServlet{
	public:
		using ptr = std::shared_ptr<FunctionWSServlet>;
		using on_connect_cb = std::function<int32_t(Routn::Http::HttpRequest::ptr header
                		, Routn::Http::WSSession::ptr session)>;
		using on_close_cb = std::function<int32_t(Routn::Http::HttpRequest::ptr header
						, Routn::Http::WSSession::ptr session)>;
		using callback = std::function<int32_t(Routn::Http::HttpRequest::ptr header
						, Routn::Http::WSFrameMessage::ptr msg
						, Routn::Http::WSSession::ptr session)>;
		FunctionWSServlet(callback cb
						, on_connect_cb connect_cb = nullptr
						, on_close_cb close_cb = nullptr);

		virtual int32_t onConnect(Routn::Http::HttpRequest::ptr header
                		, Routn::Http::WSSession::ptr session) override;
		virtual int32_t onClose(Routn::Http::HttpRequest::ptr header
						, Routn::Http::WSSession::ptr session) override;
		virtual int32_t handle(Routn::Http::HttpRequest::ptr header
						, Routn::Http::WSFrameMessage::ptr msg
						, Routn::Http::WSSession::ptr session) override;
	protected:
		callback _callback;
		on_connect_cb _onConnect;
		on_close_cb _onClose;
	};

	class WSServletDispatch : public ServletDispatch{
	public:
		using ptr = std::shared_ptr<WSServletDispatch>;
		using RWMutexType = RW_Mutex;

		WSServletDispatch()
			: ServletDispatch(){

		}
		void addServlet(const std::string& uri
						, FunctionWSServlet::callback cb
						, FunctionWSServlet::on_connect_cb connect_cb = nullptr
						, FunctionWSServlet::on_close_cb close_cb = nullptr);
		void addGlobServlet(const std::string& uri
						, FunctionWSServlet::callback cb
						, FunctionWSServlet::on_connect_cb connect_cb = nullptr
						, FunctionWSServlet::on_close_cb close_cb = nullptr);
		WSServlet::ptr getWSServlet(const std::string& uri);
	};
}
}

#endif

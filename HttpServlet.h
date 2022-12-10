/*************************************************************************
	> File Name: HttpServlet.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月22日 星期二 23时05分52秒
 ************************************************************************/

#ifndef _HTTPSERVLET_H
#define _HTTPSERVLET_H

#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "HTTP.h"
#include "../Thread.h"
#include "HttpSession.h"


namespace Routn{
namespace Http{

	class Servlet{
	public:
		using ptr = std::shared_ptr<Servlet>;
		Servlet(const std::string& name)
			: _name(name) {}
		virtual ~Servlet() {}
		virtual int32_t handle(Routn::Http::HttpRequest::ptr request, 
						Routn::Http::HttpResponse::ptr response,
						Routn::Http::HttpSession::ptr session) = 0;
		
		const std::string& getName() const { return _name;}
	protected:
		std::string _name;
	};

	class FunctionServlet : public Servlet{
	public:
		using ptr = std::shared_ptr<FunctionServlet>;
		using callback = std::function<int32_t(Routn::Http::HttpRequest::ptr request, 
						Routn::Http::HttpResponse::ptr response,
						Routn::Http::HttpSession::ptr session)>;
		FunctionServlet(callback cb);
		virtual int32_t handle(Routn::Http::HttpRequest::ptr request, 
						Routn::Http::HttpResponse::ptr response,
						Routn::Http::HttpSession::ptr session) override;
	private:
		callback _cb;
	};

	class ServletDispatch : public Servlet{
	public:
		using ptr = std::shared_ptr<ServletDispatch>;
		using RWMutexType = RW_Mutex;

		ServletDispatch();

		virtual int32_t handle(Routn::Http::HttpRequest::ptr request, 
						Routn::Http::HttpResponse::ptr response,
						Routn::Http::HttpSession::ptr session) override;

		void addServlet(const std::string& uri, Servlet::ptr slt);
		void addServlet(const std::string& uri, FunctionServlet::callback cb);
		void addGlobServlet(const std::string& uri, Servlet::ptr slt);
		void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);
	
		Servlet::ptr getDefault() const { return _default;}
		void setDefault(Servlet::ptr v) { _default = v;	/*Not thread safety*/}

		void delServlet(const std::string& uri);
		void delGlobServlet(const std::string& uri);

		Servlet::ptr getServlet(const std::string& uri);
		Servlet::ptr getGlobServlet(const std::string& uri);

		Servlet::ptr getMatchedServlet(const std::string& uri);
	protected:
		//uri(/routn/xxx) <-----> servlet 精确匹配uri
		std::unordered_map<std::string, Servlet::ptr> _datas;
		//uri(/routn/*) <-----> servlet 模糊匹配uri
		std::vector<std::pair<std::string, Servlet::ptr> > _globs;
		//默认servlet， 所有路径都没有匹配到时使用
		Servlet::ptr _default;
		RWMutexType _mutex;
	};

	//404 NOT FOUND
	class NotFoundServlet : public Servlet{
	public:
		using ptr = std::shared_ptr<NotFoundServlet>;
		NotFoundServlet();
		virtual int32_t handle(Routn::Http::HttpRequest::ptr request, 
						Routn::Http::HttpResponse::ptr response,
						Routn::Http::HttpSession::ptr session) override;
	};
}
}


#endif

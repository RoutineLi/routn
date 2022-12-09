/*************************************************************************
	> File Name: HttpServlet.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月22日 星期二 23时25分08秒
 ************************************************************************/

#include <fnmatch.h>

#include "HttpServlet.h"

namespace Routn{
namespace Http{
	FunctionServlet::FunctionServlet(callback cb)
		: Servlet("FunctionServlet")
		, _cb(cb){

	}

	int32_t FunctionServlet::handle(Routn::Http::HttpRequest::ptr request, 
					Routn::Http::HttpResponse::ptr response,
					Routn::Http::HttpSession::ptr session) {
		return _cb(request, response, session);
	}


	
	ServletDispatch::ServletDispatch()
		: Servlet("ServletDispatch"){
		_default.reset(new NotFoundServlet());
	}
	
	int32_t ServletDispatch::handle(Routn::Http::HttpRequest::ptr request, 
					Routn::Http::HttpResponse::ptr response,
					Routn::Http::HttpSession::ptr session){
		auto slt = getMatchedServlet(request->getPath()); //get uri-path
		if(slt){
			slt->handle(request, response, session);
		}
		return 0;
    }	

	void ServletDispatch::addServlet(const std::string& uri, Servlet::ptr slt){
		RWMutexType::WriteLock lock(_mutex);
		_datas[uri] = slt;
	}

	void ServletDispatch::addServlet(const std::string& uri, FunctionServlet::callback cb){
		RWMutexType::WriteLock lock(_mutex);
		_datas[uri].reset(new FunctionServlet(cb));
	}

	void ServletDispatch::addGlobServlet(const std::string& uri, Servlet::ptr slt){
		RWMutexType::WriteLock lock(_mutex);
		for(auto it = _globs.begin(); it != _globs.end(); ++it){
			if(it->first == uri){
				_globs.erase(it);
				break;
			}
		}
		_globs.push_back({uri, slt});
	}

	void ServletDispatch::addGlobServlet(const std::string& uri, FunctionServlet::callback cb){
		return addGlobServlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
	}


	void ServletDispatch::delServlet(const std::string& uri){
		RWMutexType::WriteLock lock(_mutex);
		_datas.erase(uri);
	}

	void ServletDispatch::delGlobServlet(const std::string& uri){
		RWMutexType::WriteLock lock(_mutex);
		for(auto it = _globs.begin(); it != _globs.end(); ++it){
			if(it->first == uri){
				_globs.erase(it);
				break;	
			}
		}
	}


	Servlet::ptr ServletDispatch::getServlet(const std::string& uri){
		RWMutexType::ReadLock lock(_mutex);
		auto it = _datas.find(uri);
		return it == _datas.end() ? nullptr : it->second;
	}

	Servlet::ptr ServletDispatch::getGlobServlet(const std::string& uri){
		RWMutexType::ReadLock lock(_mutex);
		for(auto it = _globs.begin(); it != _globs.end(); ++it){
			if(it->first == uri){
				return it->second;
			}
		}
		return nullptr;
	}

	Servlet::ptr ServletDispatch::getMatchedServlet(const std::string& uri){
		RWMutexType::ReadLock lock(_mutex);
		auto it = _datas.find(uri);
		if(it != _datas.end()){
			return it->second;
		}
		for(auto it = _globs.begin(); it != _globs.end(); ++it){
			if(!fnmatch(it->first.c_str(), uri.c_str(), 0)){
				return it->second;
			}
		}
		return _default;		
	}

	NotFoundServlet::NotFoundServlet()
		: Servlet("NotFoundServlet"){

	}

	int32_t NotFoundServlet::handle(Routn::Http::HttpRequest::ptr request, 
						Routn::Http::HttpResponse::ptr response,
						Routn::Http::HttpSession::ptr session){
		static const std::string& RSP_BODY = "<html><head><title>404 Not Found"
        	"</title></head><body><center><h1>404 Not Found</h1></center>"
        	"<hr><center>" + response->getHeader("Server") + "</center></body></html>";

		response->setStatus(Routn::Http::HttpStatus::NOT_FOUND);
		response->setHeader("Content-Type", "text/html");
		response->setBody(RSP_BODY);
		return 0;
	}
	
}
}


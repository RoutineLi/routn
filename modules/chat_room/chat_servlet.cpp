/*************************************************************************
	> File Name: chat_servlet.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月16日 星期五 19时57分40秒
 ************************************************************************/

#include "chat_servlet.h"
#include <iostream>
#include <fstream>

namespace Routn{
namespace Http{
	ResourceServlet::ResourceServlet(const std::string& path)
		: Servlet("ResourceServlet")
		, _path(path){

	}

	int32_t ResourceServlet::handle(Routn::Http::HttpRequest::ptr request, 
							Routn::Http::HttpResponse::ptr response,
							Routn::SocketStream::ptr session){
		
		auto path = _path.erase(_path.size() - 1) + request->getPath();
		std::cout << path << std::endl;

		if(path.find("..") != path.npos){
			response->setBody("invalid file");
			response->setStatus(Routn::Http::HttpStatus::NOT_FOUND);
			return 0;
		}
		std::ifstream ifs(path);
		if(!ifs){
			response->setBody("invalid file");
			response->setStatus(Routn::Http::HttpStatus::NOT_FOUND);
			return 0;
		}

		std::stringstream ss;
		std::string line;

		while(std::getline(ifs, line)){
			ss << line << std::endl;
		}

		response->setBody(ss.str());
		return 0;
	}
}	
}


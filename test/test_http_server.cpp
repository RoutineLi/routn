/*************************************************************************
	> File Name: test_http_server.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月21日 星期一 10时43分56秒
 ************************************************************************/

#include "../src/Routn.h"
#include "../src/http/HttpServer.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

void test(){
	Routn::Http::HttpServer::ptr server(new Routn::Http::HttpServer(true));
	Routn::Address::ptr addr = Routn::Address::LookupAnyIPAddress("0.0.0.0:8083");
	while(!server->bind(addr)){
		sleep(2);
	}
	auto sd = server->getServletDispatch();

	sd->addServlet("/routn/*", [](Routn::Http::HttpRequest::ptr req, Routn::Http::HttpResponse::ptr rsp, Routn::Http::HttpSession::ptr session){
		ROUTN_LOG_INFO(g_logger) << "req: " << L_GREEN << req->toString() << _NONE;
		rsp->setBody("Glob:\r\n" + req->toString());
		return 0;
	});

	sd->addServlet("/routn/love", [](Routn::Http::HttpRequest::ptr req, Routn::Http::HttpResponse::ptr rsp, Routn::Http::HttpSession::ptr session){
		ROUTN_LOG_INFO(g_logger) << "req: " << L_GREEN << req->toString() << _NONE;
		std::ifstream fs;
		fs.open("/home/rotun-li/routn/bin/love.html");
		std::string buff;
		if(fs.is_open()){
			std::string tmp;
			while(getline(fs, tmp)){
				buff += tmp;
			}
		}
		fs.close();
		rsp->setBody(buff);
		return 0;
	});
	server->start();
}

int main(int argc, char** argv){
	Routn::IOManager iom(2);
	iom.schedule(test);
	return 0;
}


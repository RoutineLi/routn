/*************************************************************************
	> File Name: test_ws_server.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月10日 星期六 18时26分12秒
 ************************************************************************/

#include "src/Routn.h"
#include "src/http/WSServer.h"

Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

void test(){
	Routn::Http::WSServer::ptr server(new Routn::Http::WSServer);
	Routn::Address::ptr addr = Routn::Address::LookupAnyIPAddress("172.20.10.2:8020");
	if(!addr){
		ROUTN_LOG_ERROR(g_logger) << "get addr error";
		return ;
	}	
	while(!server->bind(addr)){
		sleep(2);
	}
	server->getWSServletDispatch()->addServlet("/routn", [](Routn::Http::HttpRequest::ptr header
															, Routn::Http::WSFrameMessage::ptr msg
															, Routn::Http::WSSession::ptr session){
		session->sendMessage(msg);
		return 0;
	});
	server->start();
}

int main(int argc, char** argv){
	Routn::IOManager iom(2);
	iom.schedule(test);
	return 0;
}


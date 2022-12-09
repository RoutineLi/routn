/*************************************************************************
	> File Name: http_server_demo.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月25日 星期五 17时23分40秒
 ************************************************************************/

#include "../src/http/HttpServer.h"
#include "../src/IoManager.h"
#include "../src/Log.h"

Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();
static Routn::IOManager::ptr worker;
void run(){
    g_logger->setLevel(Routn::LogLevel::INFO);
    Routn::IPAddress::ptr addr = Routn::Address::LookupAnyIPAddress("0.0.0.0:8083");
    if(!addr){
        ROUTN_LOG_ERROR(g_logger) << "get address error";
        return ;
    }

    Routn::Http::HttpServer::ptr http_server(new Routn::Http::HttpServer(true, worker.get()));
    while(!http_server->bind(addr)){
        ROUTN_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }
    http_server->start();
}

int main(int argc, char** argv){
    Routn::IOManager iom(1);
    worker.reset(new Routn::IOManager(4, false));
    iom.schedule(run);

    return 0;
}

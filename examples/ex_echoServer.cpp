/*************************************************************************
	> File Name: ex_echoServer.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月21日 星期一 01时12分01秒
 ************************************************************************/

#include "../src/Routn.h"
#include "../src/TcpServer.h"
#include "../src/ByteArray.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

class EchoServer : public Routn::TcpServer{
public:
	EchoServer(int type){
		_type = type;
	}
protected:
	void handleClient(Routn::Socket::ptr client) override{
		ROUTN_LOG_INFO(g_logger) << "handleClient " << *client;
		Routn::ByteArray::ptr ba(new Routn::ByteArray);
		while(true){
			ba->clear();
			std::vector<iovec> iovs;
			ba->getWriteBuffers(iovs, 1024);

			int ret = client->recv(&iovs[0], iovs.size());
			if(ret == 0){
				ROUTN_LOG_INFO(g_logger) << "client close " << *client;
				break;
			}else if(ret < 0){
				ROUTN_LOG_INFO(g_logger) << "client error ret = " << ret 
					<< " errno = " << errno << " reason = " << strerror(errno);
				break;
			}
			ba->setPosition(ba->getPosition() + ret);
			ba->setPosition(0);
			if(_type == 1){
				ROUTN_LOG_INFO(g_logger) << L_GREEN << ba->toString() << _NONE;
			}else{
				ROUTN_LOG_INFO(g_logger) << L_GREEN << ba->toHexString() << _NONE;
			}
		} 
	}
private:
	int _type = 0;
};

int type = 1;

void run(){
	EchoServer::ptr es(new EchoServer(type));
	auto addr = Routn::Address::LookupAny("172.20.10.3:8083");
	while(!es->bind(addr)){
		sleep(2);
	}
	es->start();
}

int main(int argc, char** argv){
	if(argc < 2){
		fprintf(stderr, L_RED "Usage: %s -type(1: text other: hex)\n" _NONE, argv[0]);
		exit(0);
	}
	if(atoi(argv[1]) != 1){
		type = 2;
	}
	Routn::IOManager iom(2);
	iom.schedule(run);
	return 0;
}


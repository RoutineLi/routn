/*************************************************************************
	> File Name: test_ws_client.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月11日 星期日 02时12分38秒
 ************************************************************************/

#include "../src/http/WSConnection.h"
#include "../src/Routn.h"

void test(){
	auto ret = Routn::Http::WSConnection::Create("http://172.20.10.2:8020/routn", 1000);
	if(!ret.second){
		std::cout << L_BLUE << ret.first->toString() << _NONE << std::endl;
		return ;
	}

	auto conn = ret.second;
	while(true){
        for(int i = 0; i < 1; ++i) {
            conn->sendMessage(Routn::random_string(60), Routn::Http::WSFrameHead::TEXT_FRAME, false);
        }
        conn->sendMessage(Routn::random_string(65), Routn::Http::WSFrameHead::TEXT_FRAME, true);
        auto msg = conn->recvMessage();
        if(!msg) {
            break;
        }
        std::cout << "opcode=" << msg->getOpcode()
                  << " data=" << msg->getData() << std::endl;

        sleep(10);		
	}
}

int main(int argc, char** argv){
	srand(time(0));
	Routn::IOManager iom(1);
	iom.schedule(test);
	return 0;
}


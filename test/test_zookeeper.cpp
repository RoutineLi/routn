/*************************************************************************
	> File Name: test_zookeeper.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月09日 星期一 19时45分01秒
 ************************************************************************/

#include "../src/Zk_client.h"
#include "../src/Log.h"
#include "../src/IoManager.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

int g_argc;

void on_watcher(int type, int stat, const std::string& path, Routn::ZKClient::ptr client){
	ROUTN_LOG_INFO(g_logger) << " type = " << type
		<< " stat = " << stat
		<< " path = " << path
		<< " client = " << client
		<< " fiber = " << Routn::Fiber::GetThis()
		<< " iomanager = " << Routn::IOManager::GetThis();

	if(stat == ZOO_CONNECTED_STATE){
		if(g_argc == 1){
			std::vector<std::string> vals;
			Stat stat;
			int rt = client->getChildren("/", vals, true, &stat);
			if(rt == ZOK){
				ROUTN_LOG_INFO(g_logger) << "[" << Routn::Join(vals.begin(), vals.end(), ",") << "]";
			}else{
				ROUTN_LOG_INFO(g_logger) << "getChildren error " << rt;
 			}
		}else{
			std::string new_val;
			new_val.resize(255);
			int rt = client->create("/zkxxx", "", new_val, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL);
			if(rt == ZOK){
				ROUTN_LOG_INFO(g_logger) << "[" << new_val.c_str() << "]";
			}else{
				ROUTN_LOG_INFO(g_logger) << "getChildren error" << rt;
			}

			rt = client->create("/zkxxx", "", new_val, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL_SEQUENTIAL);
			if(rt == ZOK){
				ROUTN_LOG_INFO(g_logger) << "create [" << new_val.c_str() << "]";
			}else{
				ROUTN_LOG_INFO(g_logger) << "create error" << rt;
			}

			rt = client->create("/hello", "", new_val, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL);
			if(rt == ZOK){
				ROUTN_LOG_INFO(g_logger) << "get [" << new_val.c_str() << "]";
			}else{
				ROUTN_LOG_INFO(g_logger) << "get error" << rt;
			}

			rt = client->set("/hello", "xxx");
			if(rt == ZOK){
				ROUTN_LOG_INFO(g_logger) << "set [" << new_val.c_str() << "]";
			}else{
				ROUTN_LOG_INFO(g_logger) << "set error " << rt;
			}

			rt = client->del("/hello");
            if(rt == ZOK) {
                ROUTN_LOG_INFO(g_logger) << "set [" << new_val.c_str() << "]";
            } else {
                ROUTN_LOG_INFO(g_logger) << "set error " << rt;
            }

		}
	}else if(stat == ZOO_EXPIRED_SESSION_STATE){
		client->reconnect();
	}
}


int main(int argc, char** argv){
	g_argc = argc;
	Routn::IOManager iom(1);
	Routn::ZKClient::ptr client(new Routn::ZKClient);
	if(g_argc > 1){
		ROUTN_LOG_INFO(g_logger) << client->init("127.0.0.1:2181", 3000, on_watcher);
		iom.addTimer(1115000, [client](){client->close();});
	}else{
		ROUTN_LOG_INFO(g_logger) << client->init("127.0.0.1:2181", 3000, on_watcher);
 		iom.addTimer(5000, [](){}, true);
	}
	iom.stop();
	return 0;
}

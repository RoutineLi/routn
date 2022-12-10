/*************************************************************************
	> File Name: Deamon.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月27日 星期日 01时58分40秒
 ************************************************************************/

#include "Daemon.h"
#include "Log.h"
#include "Config.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");
static Routn::ConfigVar<uint32_t>::ptr g_daemon_restart_interval
	= Routn::Config::Lookup("daemon.restart_interval", (uint32_t)5, "daemon restart interval");

namespace Routn{

	static int real_start(int argc, char** argv
							, std::function<int(int argc, char** argv)> main_cb){
		return main_cb(argc, argv);
	}

	static int real_daemon(int argc, char** argv
							, std::function<int(int argc, char** argv)> main_cb){
		daemon(1, 0);	
		ProcessInfoMgr::GetInstance()->parent_id = getpid();
		ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
		while(true){
			pid_t pid = fork();
			if(pid == 0){
				//child proc
				ProcessInfoMgr::GetInstance()->main_id = getpid();
				ProcessInfoMgr::GetInstance()->main_start_time = time(0);
				ROUTN_LOG_INFO(g_logger) << "Routn-server start! pid = " << getpid();
				return real_start(argc, argv, main_cb);
			}else if(pid < 0){
				ROUTN_LOG_ERROR(g_logger) << "fork fail return = " << pid
					<< " errno = " << " reason = " << strerror(errno);
				return -1;
			}else{
				//parent proc 
				int status = 0;
				waitpid(pid, &status, 0);
				if(status){
					ROUTN_LOG_ERROR(g_logger) << "server crash pid = " << pid
						<< " errno = " << errno << " reason : " << strerror(errno);
				}else{
					ROUTN_LOG_INFO(g_logger) << "server finished pid = " << pid;
					break;
				}

				ProcessInfoMgr::GetInstance()->main_id = getpid();
				++ProcessInfoMgr::GetInstance()->restart_count;
				
				sleep(g_daemon_restart_interval->getValue());
			}
		}
		return 0;
	}

	int start_daemon(int argc, char** argv
						, std::function<int(int argc, char** argv)> main_cb
						, bool is_daemon){
		if(!is_daemon){
			return real_start(argc, argv, main_cb);
		}

		return real_daemon(argc, argv, main_cb);
	}	
}


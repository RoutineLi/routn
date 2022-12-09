/*************************************************************************
	> File Name: Deamon.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月27日 星期日 01时54分00秒
 ************************************************************************/

#ifndef _DEAMON_H
#define _DEAMON_H

#include <sys/wait.h>
#include <sys/types.h>
#include <functional>
#include <unistd.h>
#include <sstream>
#include <ctime>
#include <string>

#include "Singleton.h"
#include "Util.h"

/**
 *  守护进程模块：可以选择守护进程的方式启动server
 *  argc: 参数个数
 *  argv：参数
 *  main_cb：主函数封装
 *  is_daemon：状态
 *  返回执行结果
 */


namespace Routn{
struct ProcessInfo{
	/// 父进程id
	pid_t parent_id;
	/// 主进程id
	pid_t main_id;
	/// 父进程启动时间
	uint64_t parent_start_time = 0;
	/// 主进程启动时间
	uint64_t main_start_time = 0;
	/// 主进程重启的次数
	uint32_t restart_count = 0;
	inline std::string toString() const{
		std::stringstream ss;
		ss << "[ProcessInfo parent_id = " << parent_id
		   << " main id = " << main_id
		   << " parent_start_time = " << Routn::TimerToString(parent_start_time)
		   << " main_start_time = " << Routn::TimerToString(main_start_time)
		   << " restart_count = " << restart_count << "]";
		return ss.str();
	}
};

using ProcessInfoMgr = Routn::Singleton<ProcessInfo>;

int start_daemon(int argc, char** argv
						, std::function<int(int argc, char** argv)> main_cb
						, bool is_daemon);

}

#endif

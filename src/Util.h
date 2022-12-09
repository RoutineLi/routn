/*************************************************************************
	> File Name: Util.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月25日 星期二 18时42分15秒
 ************************************************************************/

#ifndef _UTIL_H
#define _UTIL_H

#include <boost/lexical_cast.hpp>
#include <vector>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <syscall.h>
#include <stdint.h>
#include <string>
#include <sys/stat.h>
#include <execinfo.h>

namespace Routn
{

	pid_t GetThreadId();

	uint32_t GetFiberId();

	void Backtrace(std::vector<std::string> &bt, int size, int skip = 1);

	std::string BacktraceToString(int size, int skip = 2, const std::string &prefix = " ");

	//获取高精度时间-ms
	uint64_t GetCurrentMs();
	//获取高精度时间-us
	uint64_t GetCurrentUs();

	std::string TimerToString(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");

	//文件操作相关类
	class FSUtil{
	public:
		static void ListAllFile(std::vector<std::string>& files
								, const std::string& path
								, const std::string& subfix);
	
		static bool IsRunningPidfile(const std::string& pidfile);
		static bool Mkdir(const std::string& dirname);
	};

	template<class V, class Map, class K>
	V GetParamValue(const Map& m, const K& k, const V& def = V()) {
		auto it = m.find(k);
		if(it == m.end()) {
			return def;
		}
		try {
			return boost::lexical_cast<V>(it->second);
		} catch (...) {
		}
		return def;
	}

	template<class V, class Map, class K>
	bool CheckGetParamValue(const Map& m, const K& k, V& v) {
		auto it = m.find(k);
		if(it == m.end()) {
			return false;
		}
		try {
			v = boost::lexical_cast<V>(it->second);
			return true;
		} catch (...) {
		}
		return false;
	}

};

#endif

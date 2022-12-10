/*************************************************************************
	> File Name: Env.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月27日 星期日 03时00分56秒
 ************************************************************************/

#ifndef _ENV_H
#define _ENV_H

#include <cstring>
#include <cstdlib>
#include <unistd.h> 
#include <vector>
#include <utility>
#include <iomanip>
#include <unordered_map>

#include "Singleton.h"
#include "Macro.h"
#include "Thread.h"

/**
 * 参数解析模块
 */

namespace Routn{

	class Env{
	public:
		using RWMutexType = RW_Mutex;

		bool init(int argc, char** argv);
		void del(const std::string& key);
		void add(const std::string& key, const std::string& val);
		bool hasKey(const std::string& key);
		std::string get(const std::string& key, const std::string& default_val = "");

		void addHelpInfo(const std::string& key, const std::string& desc);
		void removeHelp(const std::string& key);
		void printHelp();

		//获取绝对路径
		std::string getExe() const { return _exe;}
		//获取相对路径
		std::string getCwd() const { return _cwd;}

		bool setEnv(const std::string& key, const std::string& val);
		std::string getEnv(const std::string& key, const std::string& default_val = "");
	
		std::string getAbsolutePath(const std::string& path) const;
		std::string getConfigPath();
	private:
		RWMutexType _mutex;
		std::unordered_map<std::string, std::string> _args;
		std::vector<std::pair<std::string, std::string> > _helps;
	
		std::string _program;	//程序名称
		std::string _exe;		//程序路径
		std::string _cwd; 		//绝对路径
	};

	using EnvMgr = Routn::Singleton<Env>;
}

#endif

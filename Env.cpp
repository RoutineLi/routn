/*************************************************************************
	> File Name: Env.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月27日 星期日 03时09分25秒
 ************************************************************************/

#include "Env.h"
#include "Log.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn{

	bool Env::init(int argc, char** argv){
		//默认传参格式皆为 -x 123 -y 456
		char link[1024] = {0}, path[1024] = {0};
		sprintf(link, "/proc/%d/exe", getpid());
		readlink(link, path, sizeof(path));
		_exe = path;
		
		auto pos = _exe.find_last_of("/");
		_cwd = _exe.substr(0, pos) + "/";

		_program = argv[0];
		const char* now_key = nullptr;
		for(int i = 1; i < argc; ++i){
			if(argv[i][0] == '-'){
				if(strlen(argv[i]) > 1){
					if(now_key){
						add(now_key, "");
					}
					now_key = argv[i] + 1;
				}else{
					ROUTN_LOG_ERROR(g_logger) << "invalid arg idx = " << i
						<< " val = " << argv[i];
					return false;
				}
			}else{
				if(now_key){
					add(now_key, argv[i]);
					now_key = nullptr;
				}else{
					ROUTN_LOG_ERROR(g_logger) << "invalid arg idx = " << i
						<< " val = " << argv[i];
					return false;
				}
			}
		}
		if(now_key){
			add(now_key, "");
		}
		return true;
	}

	void Env::add(const std::string& key, const std::string& val){
		RWMutexType::WriteLock lock(_mutex);
		_args[key] = val;
	}

	bool Env::hasKey(const std::string& key){
		RWMutexType::ReadLock lock(_mutex);
		auto it = _args.find(key);
		return it != _args.end();
	}

	void Env::del(const std::string& key){
		RWMutexType::WriteLock lock(_mutex);
		_args.erase(key);
	}

	std::string Env::get(const std::string& key, const std::string& default_val){
		RWMutexType::ReadLock lock(_mutex);
		auto it = _args.find(key);
		return it != _args.end() ? it->second : default_val;
	}

	void Env::addHelpInfo(const std::string& key, const std::string& desc){
		removeHelp(key);
		RWMutexType::WriteLock lock(_mutex);
		_helps.push_back({key, desc});
	}

	void Env::removeHelp(const std::string& key){
		RWMutexType::WriteLock lock(_mutex);
		for(auto it = _helps.begin(); it != _helps.end();){
			if(it->first == key){
				it = _helps.erase(it);
			}else{
				++it;
			}
		}
	}

	void Env::printHelp(){
		RWMutexType::ReadLock lock(_mutex);
		std::cout << L_RED << "Usage: " << _program << " [options]" << _NONE << std::endl;
		for(auto &i : _helps){
			std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
		}
	}


	bool Env::setEnv(const std::string& key, const std::string& val){
		return !setenv(key.c_str(), val.c_str(), 1);
	}

	std::string Env::getEnv(const std::string& key, const std::string& default_val){
		const char* v = getenv(key.c_str());
		if(!v){
			return default_val;
		}
		return v;
	}

	std::string Env::getAbsolutePath(const std::string& path) const{
		if(path.empty()){
			return "/";
		}
		if(path[0] == '/'){
			return path;
		}
		return _cwd + path;
	}

	std::string Env::getConfigPath(){
		return getAbsolutePath(get("c", "conf"));
	}
}


/*************************************************************************
	> File Name: Library.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月28日 星期一 10时48分24秒
 ************************************************************************/

#include "Library.h"

#include <dlfcn.h>
#include "Config.h"
#include "Env.h"
#include "Log.h"

/**
 * @brief 加载业务模块动态库类
 * 
 */
static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn{

	typedef Module*(*create_module)();
	typedef void (*destroy_module)(Module*);

	struct ModuleCloser{
		ModuleCloser(void *handle, destroy_module d)
			: _handle(handle)
			, _destroy(d){

		}
		void operator()(Module* module){
			std::string name = module->getName();
			std::string version = module->getVersion();
			std::string path = module->getFilename();
			_destroy(module);
			int ret = dlclose(_handle);
			if(ret){
				ROUTN_LOG_ERROR(g_logger) << "dlclose handle fail handle = "
					<< _handle << " name = " << name
					<< " version = " << version
					<< " path = " << path
					<< " error = " << dlerror();
			}else{
				ROUTN_LOG_INFO(g_logger) << "destroy module = " << name;
			}
		}

	private:
		void* _handle;
		destroy_module _destroy;
	};

	Module::ptr Library::GetModule(const std::string& path){
		ROUTN_LOG_INFO(g_logger) << "Get Module By " << path;
		void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_DEEPBIND | RTLD_GLOBAL);
		if(!handle){
			ROUTN_LOG_ERROR(g_logger) << "connot load dl file: " << path << " reason: " << dlerror();
			return nullptr;
		}

		create_module create = (create_module)dlsym(handle, "CreateModule");
		if(!create)
		{
			ROUTN_LOG_ERROR(g_logger) << "cannot load symbol CreateModule in " << path;
			dlclose(handle);
			return nullptr;
		}

		destroy_module destroy = (destroy_module)dlsym(handle, "DestroyModule");
		if(!destroy){
			ROUTN_LOG_ERROR(g_logger) << "cannot load symbol DestroyModule in " << path;
			dlclose(handle);
			return nullptr;			
		}
		Module::ptr module(create(), ModuleCloser(handle, destroy));
		module->setFilename(path);
		Env env;
		Config::LoadFromConfDir(Routn::EnvMgr::GetInstance()->getConfigPath());
		return module;
	}

}


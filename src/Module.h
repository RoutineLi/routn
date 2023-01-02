/*************************************************************************
	> File Name: Module.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月28日 星期一 09时37分10秒
 ************************************************************************/

#ifndef _MODULE_H
#define _MODULE_H

#include "streams/Stream.h"
#include "rpc/RockStream.h"
#include "Singleton.h"
#include "Thread.h"

#include <map>
#include <memory>
#include <unordered_map>

/**
 * 业务模块化，通过以下方式调用接口
 * 
 * extern "C" {
 * Module* CreateModule() {
 *  return XX;
 * }
 * void DestoryModule(Module* ptr) {
 *  delete ptr;
 * }
 * }
 */

namespace Routn{

	class Module{
	public:
		enum Type{
			MODULE = 0,
			ROCK = 1
		};
		using ptr = std::shared_ptr<Module>;
		Module(const std::string& name
				, const std::string& version
				, const std::string& filename
				, uint32_t type = MODULE);
		virtual ~Module(){}

		virtual void onBeforeArgsParse(int argc, char** argv);
		virtual void onAfterArgsParse(int argc, char** argv);

		virtual bool handleRequest(Message::ptr req
							,Message::ptr rsp
							,Stream::ptr stream){
			return true;
		}
    	virtual bool handleNotify(Message::ptr notify
                              ,Stream::ptr stream){
			return true;
		}

		virtual bool onLoad();
		virtual bool onUnload();

		virtual bool onConnect(Routn::Stream::ptr stream);
		virtual bool onDisconnect(Routn::Stream::ptr stream);

		virtual bool onServerReady();
		virtual bool onServerUp();

		const std::string& getName() const { return _name;};
		const std::string& getVersion() const { return _version;}
		const std::string& getFilename() const { return _filename;}; 
		const std::string& getId() const { return _id;};

		void setFilename(const std::string& v) { _filename = v;}
		uint32_t getType() const { return _type;}

	protected:
		std::string _name;
		std::string _version;
		std::string _filename;
		std::string _id;
		uint32_t _type;
	};

	class RockModule : public Module{
	public:
		using ptr = std::shared_ptr<RockModule>;
		RockModule(const std::string& name
				, const std::string& version
				, const std::string& filename);
		virtual bool handleRockRequest(RockRequest::ptr request
							, RockResponse::ptr response
							, RockStream::ptr stream) = 0;
		virtual bool handleRockNotify(RockNotify::ptr request
							, RockStream::ptr stream) = 0;

		virtual bool handleRequest(Message::ptr req
							,Message::ptr rsp
							,Stream::ptr stream) override;
    	virtual bool handleNotify(Message::ptr notify
                              ,Stream::ptr stream) override;
	};

	class ModuleManager{
	public:
		using RWMutexType = RW_Mutex;
		ModuleManager();
		void add(Module::ptr m);
		void del(const std::string& name);
		void delAll();

		void init();

		Module::ptr get(const std::string& name);

		void onConnect(Stream::ptr stream);
		void onDisconnect(Stream::ptr stream);

		void listAll(std::vector<Module::ptr>& ms);
		void listByType(uint32_t type, std::vector<Module::ptr>& ms);
		void foreach(uint32_t type, std::function<void(Module::ptr)> cb);

	private:
		void initModule(const std::string& path);
	private:
		RWMutexType _mutex;
		std::unordered_map<std::string, Module::ptr> _modules;
		std::unordered_map<uint32_t, std::unordered_map<std::string, Module::ptr> > _type2Modules;
	};

	using ModuleMgr = Routn::Singleton<ModuleManager>;
}

#endif

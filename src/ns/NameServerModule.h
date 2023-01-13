/*************************************************************************
	> File Name: NameServerModule.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月09日 星期一 01时28分31秒
 ************************************************************************/

#ifndef _NAMESERVERMODULE_H
#define _NAMESERVERMODULE_H

#include "../Module.h"
#include "NameServerProtocol.h"

namespace Routn{
namespace Ns{

	class NameServerModule;
	class NSClientInfo{
	friend class NameServerModule;
	public:
		using ptr = std::shared_ptr<NSClientInfo>;
	private:
		NSNode::ptr _node;
		std::map<std::string, std::set<uint32_t> > _domain2cmds;
	};

	class NameServerModule : public RockModule{
	public:
		using ptr = std::shared_ptr<NameServerModule>;
		NameServerModule();

		virtual bool handleRockRequest(RockRequest::ptr request
									, RockResponse::ptr response
									, RockStream::ptr stream) override;
		virtual bool handleRockNotify(RockNotify::ptr notify
									, RockStream::ptr stream) override;
		virtual bool onConnect(Stream::ptr stream) override;
		virtual bool onDisconnect(Stream::ptr stream) override;
	private:
		bool handleRegister(RockRequest::ptr request, RockResponse::ptr response, RockStream::ptr stream);
		bool handleQuery(RockRequest::ptr request, RockResponse::ptr response, RockStream::ptr stream);
		bool handleTick(RockRequest::ptr request, RockResponse::ptr response, RockStream::ptr stream);
	private:
		NSClientInfo::ptr get(RockStream::ptr rs);
		void set(RockStream::ptr rs, NSClientInfo::ptr info);
		void setQueryDomain(RockStream::ptr rs, const std::set<std::string>& ds);
		void doNotify(std::set<std::string>& domains, std::shared_ptr<NotifyMessage> nty);
		std::set<RockStream::ptr> getStreams(const std::string& domain);
	private:
		NSDomainSet::ptr _domains;
		RW_Mutex _mutex;
		std::map<RockStream::ptr, NSClientInfo::ptr> _sessions;

		/// session 关注的域名
		std::map<RockStream::ptr, std::set<std::string>> _queryDomains;
		/// 域名对应关注的session
		std::map<std::string, std::set<RockStream::ptr>> _domainToSessions;
	};
}
}

#endif

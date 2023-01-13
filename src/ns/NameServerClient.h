/*************************************************************************
	> File Name: NameServerClient.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月08日 星期日 15时38分07秒
 ************************************************************************/

#ifndef _NAMESERVERCLIENT_H
#define _NAMESERVERCLIENT_H

#include "../rpc/RockStream.h"
#include "NameServerProtocol.h"

namespace Routn{
namespace Ns{

	class NSClient : public RockConnection{
	public:
		using ptr = std::shared_ptr<NSClient>;
		NSClient();
		~NSClient();
		const std::set<std::string>& getQueryDomains();
		void setQueryDomains(const std::set<std::string>& v);

		void addQueryDomain(const std::string& domain);
		void delQueryDomain(const std::string& domain);

		bool hasQueryDomain(const std::string& domain);

		RockResult::ptr query();

		void init();
		void uninit();
		NSDomainSet::ptr getDomains() const { return _domains;}
	private:
		void onQueryDomainChange();
		bool onConnect(AsyncSocketStream::ptr stream);
		bool onDisConnect(AsyncSocketStream::ptr stream);
		bool onNotify(RockNotify::ptr, RockStream::ptr);

		void onTimer();
	private:
		Routn::RW_Mutex _mutex;
		std::set<std::string> _queryDomains;
		NSDomainSet::ptr _domains;
		uint32_t _sn = 0;
		Routn::Timer::ptr _timer;
	};

}
}

#endif

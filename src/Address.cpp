/*************************************************************************
	> File Name: Address.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月05日 星期六 14时09分35秒
 ************************************************************************/

#include "Address.h"
#include "Log.h"
#include "Endian.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn
{

	std::ostream& operator<<(std::ostream& os, const Address& addr){
		return addr.insert(os);	
	}

	template <class T>
	static T CreateMask(uint32_t bits)
	{
		return (1 << (sizeof(T) * 8 - bits)) - 1;
	}

	template <class T>
	static int CountBytes(T value){
		uint32_t result = 0;
		for(; value; ++result){
			value &= value - 1;
		}
		return result;
	}

	int Address::getFamily() const
	{
		return getAddr()->sa_family;
	}
	
	Address::ptr Address::LookupAny(const std::string& host, int family, int type, int protocol){
		std::vector<Address::ptr> result;
		if(Address::Lookup(result, host, family, type, protocol)){
			return result[0];
		}
		return nullptr;
	}	
	
	std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string& host, int family, int type, int protocol){
		std::vector<Address::ptr> result;
		if(Address::Lookup(result, host, family, type, protocol)){
			for(auto& i : result){
				IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
				if(v){
					return v;
				}
			}
		}
		return nullptr;
	}

	bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host,
						 int family, int type, int protocol)
	{
		addrinfo hints, *results = nullptr, *next = nullptr;
		memset(&hints, 0, sizeof(addrinfo));
		hints.ai_flags = 0;
		hints.ai_family = family;
		hints.ai_socktype = type;
		hints.ai_protocol = protocol;
		hints.ai_addrlen = 0;
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;
		hints.ai_next = NULL;

		std::string node;
		const char* service = nullptr;
		//检查 ipv6 service
		if(!host.empty() && host[0] == '['){
			const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
			if(endipv6){
				//TODO check out of range
				if(*(endipv6 + 1) == ':'){
					service = endipv6 + 2;
				}
				node = host.substr(1, endipv6 - host.c_str() - 1);
			}
		}

		//检查 node service
		if(node.empty()){
			service = (const char*)memchr(host.c_str(), ':', host.size());
			if(service){
				if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)){
					node = host.substr(0, service - host.c_str());\
					++service;
				}
			}
		}

		if(node.empty()){
			node = host;
		}
		int error = getaddrinfo(node.c_str(), service, &hints, &results);
		if(error){
			ROUTN_LOG_ERROR(g_logger) << "Address::Lookup getaddress(" << host << ", "
				<< family << ", " << type << ") err = " << error << " reason = " 
				<< strerror(errno);
			return false;
		}

		next = results;
		while(next){
			result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
			next = next->ai_next;
		}

		freeaddrinfo(results);
		return true;
	}


	bool Address::GetInterfaceAddress(std::unordered_multimap<std::string, std::pair<Address::ptr, uint32_t> >& result, int family){
		struct ifaddrs* next = nullptr, *results = nullptr;
		if(getifaddrs(&results) != 0){
			ROUTN_LOG_ERROR(g_logger) << "Address::GetInterfaceAddress getifaddrs "
				" err = " << errno << " reason: " << strerror(errno);
			return false;
		}
		try{
			for(next = results; next; next = next->ifa_next){
				Address::ptr addr;
				uint32_t prefix_len = ~0u;
				if(family != AF_UNSPEC && family != next->ifa_addr->sa_family){
					continue;
				}
				switch (next->ifa_addr->sa_family)
				{
				case AF_INET:{
					addr = Create(next->ifa_addr, sizeof(sockaddr_in));
					uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
					prefix_len = CountBytes(netmask);
				}
					break;
				case AF_INET6:{
					addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
					in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
					prefix_len = 0;
					for(int i = 0; i < 16; ++i){
						prefix_len += CountBytes(netmask.s6_addr[i]);
					}
				}
					break;
				default:
					break;
				}
				if(addr){
					result.insert({next->ifa_name, {addr, prefix_len}});
				}
			}
		}catch(...){
			ROUTN_LOG_ERROR(g_logger) << "Address::GetInterfaceAddress exception";
			freeifaddrs(results);
			return false;
		}		 
		freeifaddrs(results);
		return true;
	}

	bool Address::GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t> >& result, const std::string& iface, int family){
		if(iface.empty() || iface == "*"){
			if(family == AF_INET || family == AF_UNSPEC){
				result.push_back({Address::ptr(new IPv4Address()), 0u});
			}
			if(family == AF_INET6 || family == AF_UNSPEC){
				result.push_back({Address::ptr(new IPv6Address()), 0u});
			}
			return true;
		}

		std::unordered_multimap<std::string, std::pair<Address::ptr, uint32_t>> results;

		if(!GetInterfaceAddress(results, family)){
			return false;
		}
		
		auto its = results.equal_range(iface);
		for(; its.first != its.second; ++its.first){
			result.push_back(its.first->second);
		}
		return true;
	}

	Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen)
	{
		if (addr == nullptr)
			return nullptr;

		Address::ptr res;
		switch (addr->sa_family)
		{
		case AF_INET:
		{
			res.reset(new IPv4Address(*(const sockaddr_in *)addr));
		}
		break;
		case AF_INET6:
		{
			res.reset(new IPv6Address(*(const sockaddr_in6 *)addr));
		}
		break;
		default:
			res.reset(new UnknownAddress(*addr));
			break;
		}
		return res;
	}

	std::string Address::toString() const
	{
		std::stringstream ss;
		insert(ss);
		return ss.str();
	}

	bool Address::operator<(const Address &rhs) const
	{
		socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
		int res = memcmp(getAddr(), rhs.getAddr(), minlen);
		if (res < 0)
		{
			return true;
		}
		else if (res > 0)
		{
			return false;
		}
		else if (getAddrLen() < rhs.getAddrLen())
		{
			return true;
		}
		return true;
	}

	bool Address::operator==(const Address &rhs) const
	{
		return getAddrLen() == rhs.getAddrLen() && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
	}

	bool Address::operator!=(const Address &rhs) const
	{
		return !(*this == rhs);
	}

	IPAddress::ptr IPAddress::Create(const char *address, uint16_t port)
	{
		addrinfo hints, *results = nullptr;
		memset(&hints, 0, sizeof(addrinfo));

		hints.ai_flags = AI_NUMERICHOST;
		hints.ai_family = AF_UNSPEC;

		int error = getaddrinfo(address, NULL, &hints, &results);
		if (error)
		{
			ROUTN_LOG_ERROR(g_logger) << "IPAddress::Create(" << address
									  << ", " << port << ") error = " << error
									  << " errno = " << errno << " reason: " << strerror(errno);
			return nullptr;
		}

		try
		{
			IPAddress::ptr res = std::dynamic_pointer_cast<IPAddress>(
				Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
			if (res)
			{
				res->setPort(port);
			}
			freeaddrinfo(results);
			return res;
		}
		catch (...)
		{
			freeaddrinfo(results);
			return nullptr;
		}
	}

	IPv4Address::IPv4Address(const sockaddr_in &address)
	{
		_addr = address;
	}

	IPv4Address::IPv4Address(uint32_t address, uint16_t port)
	{
		memset(&_addr, 0, sizeof(_addr));
		_addr.sin_family = AF_INET;
		_addr.sin_port = byteswapOnLittleEndian(port);
		_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
	}

	const sockaddr *IPv4Address::getAddr() const
	{
		return (sockaddr *)&_addr;
	}
	socklen_t IPv4Address::getAddrLen() const
	{
		return sizeof(_addr);
	}

	std::ostream &IPv4Address::insert(std::ostream &os) const
	{
		uint32_t addr = byteswapOnLittleEndian(_addr.sin_addr.s_addr);
		os << ((addr >> 24) & 0xff) << "."
		   << ((addr >> 16) & 0xff) << "."
		   << ((addr >> 8) & 0xff) << "."
		   << (addr & 0xff);
		os << ":" << byteswapOnLittleEndian(_addr.sin_port);
		return os;
	}

	IPAddress::ptr IPv4Address::broadcastAddress(uint32_t preix_len)
	{
		if (preix_len > 32)
		{
			return nullptr;
		}

		sockaddr_in baddr(_addr);
		baddr.sin_addr.s_addr |= byteswapOnLittleEndian(CreateMask<uint32_t>(preix_len));
		return IPv4Address::ptr(new IPv4Address(baddr));
	}

	IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len)
	{
		if (prefix_len > 32)
		{
			return nullptr;
		}

		sockaddr_in baddr(_addr);
		baddr.sin_addr.s_addr &= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
		return IPv4Address::ptr(new IPv4Address(baddr));
	}

	IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len)
	{
		sockaddr_in subnet;
		memset(&subnet, 0, sizeof(subnet));
		subnet.sin_family = AF_INET;
		subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
		return IPv4Address::ptr(new IPv4Address(subnet));
	}

	IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port)
	{
		IPv4Address::ptr ret(new IPv4Address);
		ret->_addr.sin_port = byteswapOnLittleEndian<uint32_t>(port);
		int res = inet_pton(AF_INET, address, &ret->_addr.sin_addr);
		if (res <= 0)
		{
			ROUTN_LOG_ERROR(g_logger) << "IPv4Address::Create(" << address << ", "
									  << port << ") ret = " << res << " errno = " << errno
									  << " reason = " << strerror(errno);
			return nullptr;
		}
		return ret;
	}

	uint32_t IPv4Address::getPort() const
	{
		return byteswapOnLittleEndian(_addr.sin_port);
	}

	void IPv4Address::setPort(uint16_t v)
	{
		_addr.sin_port = byteswapOnLittleEndian(v);
	}

	IPv6Address::IPv6Address()
	{
		memset(&_addr, 0, sizeof(_addr));
		_addr.sin6_family = AF_INET6;
	}

	IPv6Address::IPv6Address(const sockaddr_in6 &address)
	{
		_addr = address;
	}

	IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)
	{
		memset(&_addr, 0, sizeof(_addr));
		_addr.sin6_family = AF_INET6;
		_addr.sin6_port = byteswapOnLittleEndian(port);
		memcpy(&_addr.sin6_addr.s6_addr, address, 16);
	}

	IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port)
	{
		IPv6Address::ptr ret(new IPv6Address);
		ret->_addr.sin6_port = byteswapOnLittleEndian<uint32_t>(port);
		int res = inet_pton(AF_INET, address, &ret->_addr.sin6_addr);
		if (res <= 0)
		{
			ROUTN_LOG_ERROR(g_logger) << "IPv6Address::Create(" << address << ", "
									  << port << ") ret = " << res << " errno = " << errno
									  << " reason = " << strerror(errno);
			return nullptr;
		}
		return ret;
	}

	const sockaddr *IPv6Address::getAddr() const
	{
		return (sockaddr *)&_addr;
	}

	socklen_t IPv6Address::getAddrLen() const
	{
		return sizeof(_addr);
	}

	std::ostream &IPv6Address::insert(std::ostream &os) const
	{
		os << "[";
		uint16_t *addr = (uint16_t *)_addr.sin6_addr.s6_addr;
		auto used_zeros = false;
		for (size_t i = 0; i < 8; ++i)
		{
			if (addr[i] == 0 && !used_zeros)
			{
				continue;
			}
			if (i && addr[i - 1] == 0 && !used_zeros)
			{
				os << ":";
				used_zeros = true;
			}
			if (i)
			{
				os << ":";
			}
			os << (int)byteswapOnLittleEndian(addr[i]);
		}

		if (!used_zeros && addr[7] == 0)
		{
			os << "::";
		}

		//[::::1111:]
		os << "]:" << byteswapOnLittleEndian(_addr.sin6_port);
		return os;
	}

	IPAddress::ptr IPv6Address::broadcastAddress(uint32_t preix_len)
	{
		sockaddr_in6 baddr(_addr);
		baddr.sin6_addr.s6_addr[preix_len / 8] |=
			CreateMask<uint8_t>(preix_len % 8);
		for (int i = preix_len / 8 + 1; i < 16; ++i)
		{
			baddr.sin6_addr.s6_addr[i] = 0xff;
		}
		return IPv6Address::ptr(new IPv6Address(baddr));
	}

	IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len)
	{
		sockaddr_in6 baddr(_addr);
		baddr.sin6_addr.s6_addr[prefix_len / 8] &=
			CreateMask<uint8_t>(prefix_len % 8);
		for (int i = prefix_len / 8 + 1; i < 16; ++i)
		{
			baddr.sin6_addr.s6_addr[i] = 0x00;
		}
		return IPv6Address::ptr(new IPv6Address(baddr));
	}

	IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len)
	{
		sockaddr_in6 subnet;
		memset(&subnet, 0, sizeof(subnet));
		subnet.sin6_family = AF_INET6;
		subnet.sin6_addr.s6_addr[prefix_len / 8] =
			~CreateMask<uint8_t>(prefix_len % 8);
		for (uint32_t i = 0; i < prefix_len / 8; ++i)
		{
			subnet.sin6_addr.s6_addr[i] = 0xff;
		}
		return IPv6Address::ptr(new IPv6Address(subnet));
	}

	uint32_t IPv6Address::getPort() const
	{
		return byteswapOnLittleEndian(_addr.sin6_port);
	}

	void IPv6Address::setPort(uint16_t v)
	{
		_addr.sin6_port = byteswapOnLittleEndian(v);
	}

	static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un *)0)->sun_path) - 1;

	UnixAddress::UnixAddress()
	{
		memset(&_addr, 0, sizeof(_addr));
		_addr.sun_family = AF_UNIX;
		_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
	}

	UnixAddress::UnixAddress(const std::string &path)
	{
		memset(&_addr, 0, sizeof(_addr));
		_addr.sun_family = AF_UNIX;
		_length = path.size() + 1;

		if (!path.empty() && path[0] == '\0')
		{
			--_length;
		}

		if (_length > sizeof(_addr.sun_path))
		{
			throw std::logic_error("path length too long");
		}

		memcpy(_addr.sun_path, path.c_str(), _length);
		_length += offsetof(sockaddr_un, sun_path);
	}

	const sockaddr *UnixAddress::getAddr() const
	{
		return (sockaddr *)&_addr;
	}

	socklen_t UnixAddress::getAddrLen() const
	{
		return _length;
	}

	std::ostream &UnixAddress::insert(std::ostream &os) const
	{
		if (_length > offsetof(sockaddr_un, sun_path) && _addr.sun_path[0] == '\0')
		{
			return os << "\\0" << std::string(_addr.sun_path + 1, _length - offsetof(sockaddr_un, sun_path) - 1);
		}
		return os << _addr.sun_path;
	}

	UnknownAddress::UnknownAddress(int family)
	{
		memset(&_addr, 0, sizeof(_addr));
		_addr.sa_family = family;
	}

	UnknownAddress::UnknownAddress(const sockaddr &sockaddr)
	{
		_addr = sockaddr;
	}

	const sockaddr *UnknownAddress::getAddr() const
	{
		return &_addr;
	}

	socklen_t UnknownAddress::getAddrLen() const
	{
		return sizeof(_addr);
	}

	std::ostream &UnknownAddress::insert(std::ostream &os) const
	{
		os << "[UnknownAddress family = " << _addr.sa_family
		   << " data = " << _addr.sa_data;
		return os;
	}

}

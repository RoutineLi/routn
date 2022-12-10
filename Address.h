/*************************************************************************
	> File Name: Address.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月05日 星期六 13时50分03秒
 ************************************************************************/

#ifndef _ADDRESS_H
#define _ADDRESS_H

#include <memory>
#include <string>
#include <unistd.h>
#include <sstream>
#include <ifaddrs.h>
#include <netdb.h>
#include <vector>
#include <string.h>
#include <utility>
#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <iostream>

namespace Routn{

	class IPAddress;
    class Address{
	public:
		using ptr = std::shared_ptr<Address>;

		virtual ~Address() {}

		static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);
		static bool Lookup(std::vector<Address::ptr>& result, const std::string& host, int family = AF_UNSPEC, int type = 0, int protocol = 0);
		static Address::ptr LookupAny(const std::string& host, int family = AF_UNSPEC, int type = 0, int protocol = 0);
		static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host, int family = AF_UNSPEC, int type = 0, int protocol = 0);
		static bool GetInterfaceAddress(std::unordered_multimap<std::string, std::pair<Address::ptr, uint32_t> >& result, int family = AF_UNSPEC);
		static bool GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t> >& result, const std::string& iface, int family = AF_UNSPEC);
		
		int getFamily() const;

		virtual const sockaddr* getAddr() const = 0;
		virtual socklen_t getAddrLen() const = 0;

		virtual std::ostream& insert(std::ostream& os) const = 0;
		std::string toString() const;

		bool operator<(const Address& rhs) const;
		bool operator==(const Address& rhs) const;
		bool operator!=(const Address& rhs) const;
	};

	class IPAddress : public Address{
	public:
		using ptr = std::shared_ptr<IPAddress>;
		
		static IPAddress::ptr Create(const char* address, uint16_t port = 0);

		virtual IPAddress::ptr broadcastAddress(uint32_t preix_len) = 0;
		virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
		virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

		virtual uint32_t getPort() const = 0;
		virtual void setPort(uint16_t v) = 0;
	};

	class IPv4Address : public IPAddress{
	public:
		using ptr = std::shared_ptr<IPv4Address>;
		
		static IPv4Address::ptr Create(const char*address, uint16_t port = 0);

		IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
		IPv4Address(const sockaddr_in& address);
		const sockaddr* getAddr() const override;
		socklen_t getAddrLen() const override;

		std::ostream& insert(std::ostream& os) const override;

		IPAddress::ptr broadcastAddress(uint32_t preix_len) override;
		IPAddress::ptr networkAddress(uint32_t prefix_len) override;
		IPAddress::ptr subnetMask(uint32_t prefix_len) override;

		uint32_t getPort() const override;
		void setPort(uint16_t v) override;

	private:
		sockaddr_in _addr;
	};

	class IPv6Address : public IPAddress{
	public:		
		using ptr = std::shared_ptr<IPv6Address>;
		static IPv6Address::ptr Create(const char* address, uint16_t port = 0);
		IPv6Address();
		IPv6Address(const sockaddr_in6& address);
		IPv6Address(const uint8_t address[16], uint16_t port = 0);
		const sockaddr* getAddr() const override;
		socklen_t getAddrLen() const override;
		std::ostream& insert(std::ostream& os) const override;

		IPAddress::ptr broadcastAddress(uint32_t preix_len) override;
		IPAddress::ptr networkAddress(uint32_t prefix_len) override;
		IPAddress::ptr subnetMask(uint32_t prefix_len) override;

		uint32_t getPort() const override;
		void setPort(uint16_t v) override;
	private:
		sockaddr_in6 _addr;
	};

	class UnixAddress : public Address{
	public:
		using ptr = std::shared_ptr<UnixAddress>;
		UnixAddress(const std::string& path);
		UnixAddress();
		const sockaddr* getAddr() const override;
		socklen_t getAddrLen() const override;
		void setAddrLen(uint32_t v) { _length = v; }
		std::ostream& insert(std::ostream& os) const override;
	private:
		struct sockaddr_un _addr;
		socklen_t _length;
	};

	class UnknownAddress : public Address{
	public:
		using ptr = std::shared_ptr<UnknownAddress>;	
		UnknownAddress(int family);
		UnknownAddress(const sockaddr& addr);
		const sockaddr* getAddr() const override;
		socklen_t getAddrLen() const override;
		std::ostream& insert(std::ostream& os) const override;
	private:
		sockaddr _addr;
	};

	std::ostream& operator<<(std::ostream& os, const Address& addr);
}

#endif

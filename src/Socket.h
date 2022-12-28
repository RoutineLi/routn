/*************************************************************************
	> File Name: Socket.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月06日 星期日 14时34分54秒
 ************************************************************************/

#ifndef _SOCKET_H
#define _SOCKET_H

#include <memory>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include "Address.h"
#include "Noncopyable.h"

namespace Routn{

	class Socket : public std::enable_shared_from_this<Socket>, public Noncopyable{
	public:
		using ptr = std::shared_ptr<Socket>;
		using weak_ptr = std::weak_ptr<Socket>;

		enum Type{
			TCP = SOCK_STREAM,
			UDP = SOCK_DGRAM
		};

		enum Family{
			IPV4 = AF_INET,
			IPV6 = AF_INET6,
			UNIX = AF_UNIX
		};

		static Socket::ptr CreateTCP(Routn::Address::ptr address);
		static Socket::ptr CreateUDP(Routn::Address::ptr address);

		static Socket::ptr CreateTCPsocket();
		static Socket::ptr CreateUDPsocket();

		static Socket::ptr CreateTCPsocket6();
		static Socket::ptr CreateUDPsocket6();

		static Socket::ptr CreateUnixTCPsocket();
		static Socket::ptr CreateUnixUDPsocket();

		Socket(int family, int type, int protocol = 0);
		virtual ~Socket();

		int64_t getSendTimeout();
		bool setSendTimeout(int64_t v);

		int64_t getRecvTimeout();
		bool setRecvTimeout(int64_t v);
	
		bool getOption(int level, int option, void* result, size_t* len);
		template<class T>
		bool getOption(int level, int option, T& result){
			size_t length = sizeof(T);
			return getOption(level, option, &result, &length);
		}

		bool setOption(int level, int option, const void* result, size_t len);
		template<class T>
		bool setOptionImpl(int level, int option, const T& value){
			return setOption(level, option, &value, sizeof(T));
		}

		virtual Socket::ptr accept();


		virtual bool bind(const Address::ptr addr);
		virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
		virtual bool reconnect(uint64_t timeout_ms = -1);
		virtual bool listen(int backlog = SOMAXCONN);
		virtual bool close();

		virtual int send(const void* buffer, size_t length, int flags = 0);
		virtual int send(const iovec* buffers, size_t length, int flags = 0);
		virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);
		virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);


		virtual int recv(void* buffer, size_t length, int flags = 0);
		virtual int recv(iovec* buffers, size_t length, int flags = 0);
		virtual int recvFrom(void *buffer, size_t length, Address::ptr from, int flags = 0);
		virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0);

		Address::ptr getRemoteAddress();
		Address::ptr getLocalAddress();

		int getFamily() const { return _family; }
		int getType() const { return _type; }
		int getProtocol() const { return _protocol; }

		bool isConnected() const {return _isConnected; }
		bool isValid() const;
		int getError();

		virtual std::ostream& dump(std::ostream& os) const;
		int getSocket() const { return _sock; }

		bool cancelRead();
		bool cancelWrite();
		bool cancelAccept();
		bool cancelAll();
		
		bool checkConnected();
	protected:
		virtual bool init(int sock);
		void initSock();
		void newSock();
	protected:
		int _sock;
		int _family;
		int _type;
		int _protocol;
		bool _isConnected;

		Address::ptr _localAddress;
		Address::ptr _remoteAddress;
	};

	class SSLSocket : public Socket{
	public:
		using ptr = std::shared_ptr<SSLSocket>;

		static SSLSocket::ptr CreateTCP(Routn::Address::ptr address);
		static SSLSocket::ptr CreateTCPSocket();
		static SSLSocket::ptr CreateTCPSocket6();

		SSLSocket(int family, int type, int protocol = 0);
		virtual Socket::ptr accept() override;
		virtual bool bind(const Address::ptr addr) override;
		virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1) override;
		virtual bool listen(int backlog = SOMAXCONN) override;
		virtual bool close() override;

		virtual int send(const void* buffer, size_t length, int flags = 0) override;
		virtual int send(const iovec* buffers, size_t length, int flags = 0) override;
		virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0) override;
		virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0) override;


		virtual int recv(void* buffer, size_t length, int flags = 0) override;
		virtual int recv(iovec* buffers, size_t length, int flags = 0) override;
		virtual int recvFrom(void *buffer, size_t length, Address::ptr from, int flags = 0) override;
		virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0) override;		

		bool loadCertificates(const std::string& cert_file, const std::string& key_file);
		virtual std::ostream& dump(std::ostream& os) const override;

	protected:
		virtual bool init(int sock) override;
	private:
		std::shared_ptr<SSL_CTX> _ctx;
		std::shared_ptr<SSL> _ssl;
	};

	std::ostream& operator<<(std::ostream& os, const Socket& addr);

}

#endif

/*************************************************************************
	> File Name: Socket.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月06日 星期日 14时55分59秒
 ************************************************************************/

#include "Socket.h"
#include "FdManager.h"
#include "Hook.h"
#include "Log.h"
#include "Macro.h"

#include <cstdint>


static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn{
	/**
	 * @brief Socket基类
	 * 
	 * @param address 
	 * @return Socket::ptr 
	 */

	Socket::ptr Socket::CreateTCP(Routn::Address::ptr address){
		Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
		return sock;
	}

	Socket::ptr Socket::CreateUDP(Routn::Address::ptr address){
		Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
		return sock;
	}


	Socket::ptr Socket::CreateTCPsocket(){
		Socket::ptr sock(new Socket(IPV4, TCP, 0));
		return sock;
	}

	Socket::ptr Socket::CreateUDPsocket(){
		Socket::ptr sock(new Socket(IPV4, UDP, 0));
		return sock;
	}


	Socket::ptr Socket::CreateUnixTCPsocket(){
		Socket::ptr sock(new Socket(UNIX, TCP, 0));
		return sock;
	}

	Socket::ptr Socket::CreateUnixUDPsocket(){
		Socket::ptr sock(new Socket(UNIX, UDP, 0));
		return sock;
	}

	Socket::ptr Socket::CreateTCPsocket6(){
		Socket::ptr sock(new Socket(IPV6, TCP, 0));
		return sock;
	}

	Socket::ptr Socket::CreateUDPsocket6(){
		Socket::ptr sock(new Socket(IPV6, UDP, 0));
		return sock;
	}

	Socket::Socket(int family, int type, int protocol)
		: _sock(-1)
		, _family(family)
		, _type(type)
		, _protocol(protocol)
		, _isConnected(false)
	{

	}

	Socket::~Socket()
	{
		close();
	}

	int64_t Socket::getSendTimeout(){
		FdCtx::ptr ctx = FdMgr::GetInstance()->get(_sock);
		if(ctx){
			return ctx->getTimeout(SO_SNDTIMEO);
		}
		return -1;
	}

	bool Socket::setSendTimeout(int64_t v){
		struct timeval tv;
		tv.tv_sec = (int)(v / 1000); 
		tv.tv_usec = 0;
		bool ret = setOptionImpl(SOL_SOCKET, SO_SNDTIMEO, tv);		
		return ret;
	}

	int64_t Socket::getRecvTimeout(){
		FdCtx::ptr ctx = FdMgr::GetInstance()->get(_sock);
		if(ctx){
			return ctx->getTimeout(SO_RCVTIMEO);
		}
		return -1;		
	}

	bool Socket::setRecvTimeout(int64_t v){
		struct timeval tv;
		tv.tv_sec = (int)(v / 1000); 
		tv.tv_usec = 0;
		
		bool ret = setOptionImpl(SOL_SOCKET, SO_RCVTIMEO, tv);
		return ret;
	}

	bool Socket::getOption(int level, int option, void* result, size_t* len){
		int ret = getsockopt(_sock, level, option, result, (socklen_t*)len);
		if(ret){
			ROUTN_LOG_DEBUG(g_logger) << "getOption sock = " << _sock
				<< " level = " << level << " option = "
				<< option << " errno = " << errno << " reason: " << strerror(errno);
			return false;
		}
		return true;
	}

	bool Socket::setOption(int level, int option, const void* result, size_t len){
		if(setsockopt(_sock, level, option, result, (socklen_t)len)){			
			ROUTN_LOG_DEBUG(g_logger) << "getOption sock = " << _sock
				<< " level = " << level << " option = "
				<< option << " errno = " << errno << " reason: " << strerror(errno);
			return false;
		}
		return true;
	}


	Socket::ptr Socket::accept(){
		Socket::ptr sock(new Socket(_family, _type, _protocol));
		int newsock = ::accept(_sock, nullptr, nullptr);
		if(newsock == -1){
			ROUTN_LOG_ERROR(g_logger) << "accept(" << _sock << ") errno = "
				<< errno << " reason: " << strerror(errno);
			return nullptr;
		}
		if(sock->init(newsock)){
			return sock;
		}
		return nullptr;
	}
	
	bool Socket::init(int sock){
		FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
		if(ctx && ctx->isSocket() && !ctx->isClose()){
			_sock = sock;
			_isConnected = true;
			initSock();
			getLocalAddress();
			getRemoteAddress();
			return true;
		}
		return false;
	}


	bool Socket::bind(const Address::ptr addr){
		_localAddress = addr;
		if(!isValid()){
			newSock();
			if(ROUTN_UNLIKELY(!isValid())){
				return false;
			}
		}
		if(ROUTN_UNLIKELY(addr->getFamily() != _family)){
			ROUTN_LOG_ERROR(g_logger) << "bind sock.family("
				<< _family << ") addr.family (" << addr->getFamily()
				<< ") not equal, addr = " << addr->toString();
			return false;
		}

//		UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
//		if(uaddr) {
//			Socket::ptr sock = Socket::CreateUnixTCPsocket();
//			if(sock->connect(uaddr)) {
//				return false;
//			}
//		}
//
		if(::bind(_sock, addr->getAddr(), addr->getAddrLen())){
			ROUTN_LOG_ERROR(g_logger) << "bind error! errno = " << errno
				<< " reason: " << strerror(errno);
			return false;
		}
		getLocalAddress();
		return true;
	}

	bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms){
		if(!isValid()){
			newSock();
			if(ROUTN_UNLIKELY(!isValid())){
				return false;
			}
		}
		if(ROUTN_UNLIKELY(addr->getFamily() != _family)){
			ROUTN_LOG_ERROR(g_logger) << "connect sock.family("
				<< _family << ") addr.family (" << addr->getFamily()
				<< ") not equal, addr = " << addr->toString();
			return false;
		}
		if(timeout_ms == (uint64_t)-1){
			if(::connect(_sock, addr->getAddr(), addr->getAddrLen())){
				ROUTN_LOG_ERROR(g_logger) << "sock = " << _sock << " connect(" 
					<< addr->toString() << ") error errno = " << errno
					<< " reason: " << strerror(errno);
				close();
				return false;
			}
		}else{
			if(::connect_with_timeout(_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)){
				ROUTN_LOG_ERROR(g_logger) << "sock = " << _sock << " connect(" 
					<< addr->toString() << ") error errno = " << errno
					<< " reason: " << strerror(errno);
				close();
				return false;				
			}
		}
		_isConnected = true;
		getRemoteAddress();
		getLocalAddress();
		return true;
	}

	bool Socket::reconnect(uint64_t timeout_ms){
		if(!_localAddress){
			ROUTN_LOG_ERROR(g_logger) << "reconnect local_addr is null !";
			return false;
		}
		return connect(_localAddress, timeout_ms);
	}

	bool Socket::listen(int backlog){
		if(!isValid()){
			ROUTN_LOG_ERROR(g_logger) << "listen error sock = -1";
			return false; 
		}
		if(::listen(_sock, backlog)){
			ROUTN_LOG_ERROR(g_logger) << "listen error! errno = " << errno
				<< " reason: " << strerror(errno);
			return false;
		}
		return true;
	}

	bool Socket::close(){
		if(!_isConnected && _sock == -1){
			return true;
		}
		_isConnected = false;
		if(_sock != -1){
			::close(_sock);
			_sock = -1;
		}
		return false;
	}

	int Socket::send(const void* buffer, size_t length, int flags){
		if(isConnected()){
			return ::send(_sock, buffer, length, flags);
		}
		return -1;
	}

	int Socket::send(const iovec* buffers, size_t length, int flags){
		if(isConnected()){
			msghdr msg;
			memset(&msg, 0, sizeof(msghdr));
			msg.msg_iov = (iovec*)buffers;
			msg.msg_iovlen = length;
			return ::sendmsg(_sock, &msg, flags);
		}
		return -1;
	}

	int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags){
		if(isConnected()){
			return ::sendto(_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
		}
		return -1;
	}

	int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags){
		if(isConnected()){
			msghdr msg;
			memset(&msg, 0, sizeof(msghdr));
			msg.msg_iov = (iovec*)buffers;
			msg.msg_iovlen = length;
			msg.msg_name = const_cast<sockaddr*>(to->getAddr());
			msg.msg_namelen = to->getAddrLen();
			return ::sendmsg(_sock, &msg, flags);
		}
		return -1;
	}


	int Socket::recv(void* buffer, size_t length, int flags){
		if(isConnected()){
			return ::recv(_sock, buffer, length, flags);
		}
		return -1;
	}

	int Socket::recv(iovec* buffers, size_t length, int flags){
		if(isConnected()){
			msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = (iovec*)buffers;
			msg.msg_iovlen = length;
			return ::recvmsg(_sock, &msg, flags);
		}
		return -1;
	}

	int Socket::recvFrom(void *buffer, size_t length, Address::ptr from, int flags){
		if(isConnected()){
			auto len = from->getAddrLen();
			return ::recvfrom(_sock, buffer, length, flags, const_cast<sockaddr*>(from->getAddr()), &len);
		}
		return -1;
	}

	int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags){
		if(isConnected()){
			msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = (iovec*)buffers;
			msg.msg_iovlen = length;
			msg.msg_name = const_cast<sockaddr*>(from->getAddr());
			msg.msg_namelen = from->getAddrLen();
			return ::recvmsg(_sock, &msg, flags);
		}
		return -1;
	}


	Address::ptr Socket::getRemoteAddress(){
		if(_remoteAddress){
			return _remoteAddress;
		}
		Address::ptr result;
		switch (_family)
		{
		case AF_INET:{
			result.reset(new IPv4Address());
		}
			break;
		case AF_INET6:{
			result.reset(new IPv6Address());
		}
			break;
		case AF_UNIX:{
			result.reset(new UnixAddress());
		}
			break;
		default:
			result.reset(new UnknownAddress(_family));
			break;
		}
		socklen_t addrlen = result->getAddrLen();
		if(getpeername(_sock, const_cast<sockaddr*>(result->getAddr()), &addrlen)){
			ROUTN_LOG_ERROR(g_logger) << "getpeername error sock = " << _sock
				<< " errno= " << errno << " reason: " << strerror(errno);
			return Address::ptr(new UnknownAddress(_family));
		}
		if(_family == AF_UNIX){
			UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
			addr->setAddrLen(addrlen);
		}
		_remoteAddress = result;
		return _remoteAddress;
	}
	Address::ptr Socket::getLocalAddress(){
		if(_localAddress){
			return _localAddress;
		}
		Address::ptr result;
		switch (_family)
		{
		case AF_INET:{
			result.reset(new IPv4Address());
		}
			break;
		case AF_INET6:{
			result.reset(new IPv6Address());
		}
			break;
		case AF_UNIX:{
			result.reset(new UnixAddress());
		}
			break;
		default:
			result.reset(new UnknownAddress(_family));
			break;
		}
		socklen_t addrlen = result->getAddrLen();
		if(getsockname(_sock, const_cast<sockaddr*>(result->getAddr()), &addrlen)){
			ROUTN_LOG_ERROR(g_logger) << "getsockname error sock = " << _sock
				<< " errno= " << errno << " reason: " << strerror(errno);
			return Address::ptr(new UnknownAddress(_family));
		}
		if(_family == AF_UNIX){
			UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
			addr->setAddrLen(addrlen);
		}
		_localAddress = result;
		return _localAddress;		
	}

	bool Socket::isValid() const{
		return _sock != -1;
	}

	int Socket::getError(){
		int error = 0;
		size_t len = sizeof(error);
		if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)){
			return -1;
		}	
		return error;
	}

	std::ostream& Socket::dump(std::ostream& os) const{
		os << "[Socket fd = " << _sock
		   << " is_connected = " << _isConnected
		   << " family = " << _family
		   << " type = " << _type
		   << " protocol = " << _protocol;
		if(_localAddress){
			os << " local_Address = " << _localAddress->toString();
		}
		if(_remoteAddress){
			os << " remote_Address = " << _remoteAddress->toString();
		}
		os << "]";
		return os;
	}
	
	std::string Socket::toString(){
		std::stringstream ss;
		dump(ss);
		return ss.str();
	}
	
	bool Socket::cancelRead(){
		return IOManager::GetThis()->cancelEvent(_sock, Routn::IOManager::READ);
	}

	bool Socket::cancelWrite(){
		return IOManager::GetThis()->cancelEvent(_sock, Routn::IOManager::WRITE);

	}

	bool Socket::cancelAccept(){
		return IOManager::GetThis()->cancelEvent(_sock, Routn::IOManager::READ);

	}

	bool Socket::cancelAll(){
		return IOManager::GetThis()->cancelAll(_sock);

	}

	void Socket::initSock(){
		int val = 1;
		setOptionImpl(SOL_SOCKET, SO_REUSEADDR, val);
		if(_type == SOCK_STREAM){
			setOptionImpl(IPPROTO_TCP, TCP_NODELAY, val);
		}
	}

	void Socket::newSock(){
		_sock = socket(_family, _type, _protocol);
		if(ROUTN_UNLIKELY(_sock != -1)){
			initSock();
		}else{
			ROUTN_LOG_ERROR(g_logger) << "socket(" << _family
				<< ", " << _type << ", " << _protocol << ") errno = "
				<< errno << " reason: " << strerror(errno);
		}
	}

	bool Socket::checkConnected() {
		struct tcp_info info;
		int len = sizeof(info);
		getsockopt(_sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
		_isConnected = (info.tcpi_state == TCP_ESTABLISHED);
		return _isConnected;
	}

	std::ostream& operator<<(std::ostream& os, const Socket& addr){
		return addr.dump(os);
	}

namespace{
	struct _SSLInit {
		_SSLInit(){
			SSL_library_init();
			SSL_load_error_strings();
			OpenSSL_add_all_algorithms();
		}
	};

	static _SSLInit _s_ssl_init_;
}

/**
 * @brief SSL-Socket类
 * 
 */


	SSLSocket::ptr SSLSocket::CreateTCP(Routn::Address::ptr address){
		return std::make_shared<SSLSocket>(address->getFamily(), Socket::TCP, 0);
	}

	SSLSocket::ptr SSLSocket::CreateTCPSocket(){
		return std::make_shared<SSLSocket>(Socket::IPV4, Socket::TCP, 0);
	}

	SSLSocket::ptr SSLSocket::CreateTCPSocket6(){
		return std::make_shared<SSLSocket>(Socket::IPV6, Socket::TCP, 0);
	}


	SSLSocket::SSLSocket(int family, int type, int protocol)
		: Socket(family, type, protocol){

	}

	Socket::ptr SSLSocket::accept(){
		SSLSocket::ptr sock = std::make_shared<SSLSocket>(_family, _type, _protocol);
		int newsock = ::accept(_sock, nullptr, nullptr);
		if(newsock == -1){
			ROUTN_LOG_ERROR(g_logger) << "accept(" << _sock << ") errno = "
				<< errno << " reason = " << strerror(errno);
			return nullptr;
		}
		sock->_ctx = _ctx;
		if(sock->init(newsock)){
			return sock;
		}
		return nullptr;
	}

	bool SSLSocket::bind(const Address::ptr addr){
		return Socket::bind(addr);
	}

	bool SSLSocket::connect(const Address::ptr addr, uint64_t timeout_ms ){
		bool v = Socket::connect(addr, timeout_ms);
		if(v){
			_ctx.reset(SSL_CTX_new(SSLv23_client_method()), SSL_CTX_free);
			_ssl.reset(SSL_new(_ctx.get()), SSL_free);
			SSL_set_fd(_ssl.get(), _sock);
			v = (SSL_connect(_ssl.get()) == 1);
		}
		return v;
	}

	bool SSLSocket::listen(int backlog){
		return Socket::listen(backlog);
	}

	bool SSLSocket::close(){
		return Socket::close();
	}

	int SSLSocket::send(const void* buffer, size_t length, int flags){
		if(_ssl){
			return SSL_write(_ssl.get(), buffer, length);
		}
		return -1;
	}

	int SSLSocket::send(const iovec* buffers, size_t length, int flags){
		if(!_ssl){
			return -1;
		}
		int total = 0;
		for(size_t i = 0; i < length; ++i){
			int tmp = SSL_write(_ssl.get(), buffers[i].iov_base, buffers[i].iov_len);
			if(tmp <= 0){
				return tmp;
			}
			total += tmp;
			if(tmp != (int)buffers[i].iov_len){
				break;
			}
		}
		return total;
	}

	int SSLSocket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags){
		ROUTN_ASSERT(false);
		return -1;
	}

	int SSLSocket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags){
		ROUTN_ASSERT(false);
		return -1;
	}



	int SSLSocket::recv(void* buffer, size_t length, int flags){
		if(_ssl){
			return SSL_read(_ssl.get(), buffer, length);
		}
		return -1;
	}

	int SSLSocket::recv(iovec* buffers, size_t length, int flags ){
		if(!_ssl){
			return -1;
		}
		int total = 0;
		for(size_t i = 0; i < length; ++i){
			int tmp = SSL_read(_ssl.get(), buffers[i].iov_base, buffers[i].iov_len);
			if(tmp <= 0){
				return tmp;
			}
			total += tmp;
			if(tmp != (int)buffers[i].iov_len){
				break;
			}
		}
		return total;
	}

	int SSLSocket::recvFrom(void *buffer, size_t length, Address::ptr from, int flags) {
		ROUTN_ASSERT(false);
		return -1;
	}

	int SSLSocket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags){
		ROUTN_ASSERT(false);
		return -1;
	} 	


	bool SSLSocket::loadCertificates(const std::string& cert_file, const std::string& key_file){
		_ctx.reset(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);
		if(SSL_CTX_use_certificate_chain_file(_ctx.get(), cert_file.c_str()) != 1){
			ROUTN_LOG_ERROR(g_logger) << "SSL_CTX_use_certificate_chain_file("
				<< cert_file << ") error";
			return false;
		}
		if(SSL_CTX_use_PrivateKey_file(_ctx.get(), key_file.c_str(), SSL_FILETYPE_PEM) != 1){
			ROUTN_LOG_ERROR(g_logger) << "SSL_CTX_use_PrivateKey_file("
				<< key_file << ") error";
			return false;
		}
		if(SSL_CTX_check_private_key(_ctx.get()) != 1){
			ROUTN_LOG_ERROR(g_logger) << "SSL_CTX_check_private_key cert_file = "
				<< cert_file << " key_file = " << key_file;
			return false;
		}
		return true;
	}
	
	std::ostream& SSLSocket::dump(std::ostream& os) const{
		os << "[SSLSocket sock=" << _sock
		<< " is_connected=" << _isConnected
		<< " family=" << _family
		<< " type=" << _type
		<< " protocol=" << _protocol;
		if(_localAddress) {
			os << " local_address=" << _localAddress->toString();
		}
		if(_remoteAddress) {
			os << " remote_address=" << _remoteAddress->toString();
		}
		os << "]";
		return os;		
	}


	bool SSLSocket::init(int sock){
		bool v = Socket::init(sock);
		int ret = 0;
		if(v){
			_ssl.reset(SSL_new(_ctx.get()), SSL_free);
			SSL_set_fd(_ssl.get(), _sock);
			while((ret = SSL_accept(_ssl.get())) <= 0){
				int e;
				if((e = SSL_get_error(_ssl.get(), ret)) != SSL_ERROR_WANT_READ){
					ROUTN_LOG_ERROR(g_logger) << "SSL_accept ret = " << ret
						<< " err = " << ERR_error_string(e, nullptr) << ", reason:" << SSL_state_string_long(_ssl.get());					
					v = false;
					break;
				}
			}
		}
		return v;
	}

}


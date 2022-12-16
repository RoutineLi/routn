/*************************************************************************
	> File Name: TcpServer.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月18日 星期五 22时14分27秒
 ************************************************************************/

#include "TcpServer.h"
#include "Config.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");


namespace Routn{
	static Routn::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = 
		Routn::Config::Lookup<uint64_t>("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2), "tcp server read timeoout");

	TcpServer::TcpServer(Routn::IOManager* worker, Routn::IOManager* io_worker, Routn::IOManager* accept_worker)
		: _worker(worker)
		, _io_worker(io_worker)
		, _accept_worker(accept_worker)
		, _readTimeout(g_tcp_server_read_timeout->getValue())
		, _name("routn/1.0.0")
		, _isStop(true){
	}

	TcpServer::~TcpServer(){
		for(auto &i : _socks)
			i->close();
		_socks.clear();
	}

///TODO
	bool TcpServer::bind(Routn::Address::ptr addr, bool ssl){
		std::vector<Address::ptr> addrs;
		std::vector<Address::ptr> fails;
		addrs.push_back(addr);
		return bind(addrs, fails, ssl);
	}
	
	bool TcpServer::bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fail_addrs, bool ssl){
		_ssl = ssl;
		for(auto &addr : addrs){
			Socket::ptr sock = ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
			if(!sock->bind(addr)){
				ROUTN_LOG_ERROR(g_logger) << "bind fail errno = "
					<< errno << " reason = " << strerror(errno)
					<< " addr = [" << addr->toString() << "]";
				fail_addrs.push_back(addr);
				continue;
			}
			if(!sock->listen()){
				ROUTN_LOG_ERROR(g_logger) << "listen fail errno = "
					<< errno << " reason = " << strerror(errno)
					<< " addr = [" << addr->toString() << "]";
				fail_addrs.push_back(addr);
				continue;
			}
			_socks.push_back(sock);
		}
		if(!fail_addrs.empty()){
			_socks.clear();
			return false;
		}
		
		for(auto& i : _socks){
			ROUTN_LOG_INFO(g_logger) << "server bind success: " << *i;
		}
		return true;
	}
	
	bool TcpServer::start(){
		if(!_isStop){
			return true;
		}
		_isStop = false;
		for(auto &sock : _socks){
			_accept_worker->schedule(std::bind(&TcpServer::startAccept, 
								shared_from_this(), sock));
		}
		return true;
	}
	
	void TcpServer::stop(){
		_isStop = true;
		auto self = shared_from_this();
		_accept_worker->schedule([this, self](){
			for(auto &sock : _socks){
				sock->cancelAll();
				sock->close();
			}
			_socks.clear();
		});
	}


	void TcpServer::handleClient(Socket::ptr client){
		ROUTN_LOG_INFO(g_logger) << "handle Client: " << *client;
	}


	void TcpServer::startAccept(Socket::ptr sock){
		while(!_isStop){
			Socket::ptr client = sock->accept();
			if(client){
				client->setRecvTimeout(_readTimeout);
				_io_worker->schedule(std::bind(&TcpServer::handleClient
							, shared_from_this(), client));
			}else{
				ROUTN_LOG_ERROR(g_logger) << "accept errno = " << errno
					<< " reason = " << strerror(errno);
			}
		}
	}


    bool TcpServer::loadCertificates(const std::string& cert_file, const std::string& key_file){
		for(auto &i : _socks){
			auto ssl_sock = std::dynamic_pointer_cast<SSLSocket>(i);
			if(ssl_sock){
				if(!ssl_sock->loadCertificates(cert_file, key_file)){
					return false;
				}
			}
		}
		return true;
	}
}


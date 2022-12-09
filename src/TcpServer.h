/*************************************************************************
	> File Name: TcpServer.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月18日 星期五 22时04分22秒
 ************************************************************************/

#ifndef _TCPSERVER_H
#define _TCPSERVER_H

#include <memory>
#include <map>
#include <vector>
#include <functional>

#include "IoManager.h"
#include "Config.h"
#include "Socket.h"
#include "Address.h"
#include "Noncopyable.h"

namespace Routn{


	struct TcpServerConf{
		using ptr = std::shared_ptr<TcpServerConf>;
		std::vector<std::string> address;
		bool keepalive = false;
		int timeout = 1000 * 2 * 60;
		///int ssl = 0;		//ssl协议状态
		
		///server类型：默认http
		std::string type = "http";
		std::string name;
		std::string id;
		///std::string cert_file;	//证书目录
		///std::string key_file;	//密钥目录
		std::string accept_worker;
		std::string io_worker;
		std::string process_worker;
    	std::map<std::string, std::string> args;

		std::string toString() const {
			std::stringstream ss;
			ss << "{HttpServerConf address = ";
			for(auto i : address){
				ss << i << ", ";
			}
			ss << " keepalive = " << keepalive
			   << " timeout = " << timeout
			   << " name = " << name
			   << " type = " << type;
	
			return ss.str(); 
		}

		bool isValid() const{
			return !address.empty();
		}
		bool operator==(const TcpServerConf& oth) const{
			return address == oth.address
				&& keepalive == oth.keepalive
				&& timeout == oth.timeout
				&& name == oth.name
				&& accept_worker == oth.accept_worker
				&& io_worker == oth.io_worker
				&& process_worker == oth.process_worker
				&& args == oth.args
				&& id == oth.id
				&& type == oth.type;
		}
	};

	template<>
	class LexicalCast<std::string, TcpServerConf>{
	public:
		TcpServerConf operator()(const std::string& v){
			YAML::Node node = YAML::Load(v);
			TcpServerConf conf;
			conf.id = node["id"].as<std::string>(conf.id);
			conf.keepalive = node["keepalive"].as<bool>(conf.keepalive);
			conf.timeout = node["timeout"].as<int>(conf.timeout);
			conf.name = node["name"].as<std::string>(conf.name);
			conf.type = node["type"].as<std::string>(conf.type);
			conf.accept_worker = node["accept_worker"].as<std::string>();
			conf.io_worker = node["io_worker"].as<std::string>();
			conf.process_worker = node["process_worker"].as<std::string>();
			conf.args = LexicalCast<std::string, std::map<std::string, std::string> > ()(node["args"].as<std::string>(""));			
			if(node["address"].IsDefined()){
				for(size_t i = 0; i < node["address"].size(); ++i){
					conf.address.push_back(node["address"][i].as<std::string>());
				}
			}
			return conf;
		}
	};
	template<>
	class LexicalCast<TcpServerConf, std::string>{
	public:
		std::string operator()(const TcpServerConf& conf){
			YAML::Node node;
			node["id"] = conf.id;
			node["type"] = conf.type;
			node["name"] = conf.name;
			node["keepalive"] = conf.keepalive;
			node["timeout"] = conf.timeout;
			node["accept_worker"] = conf.accept_worker;
			node["io_worker"] = conf.io_worker;
			node["process_worker"] = conf.process_worker;
			node["args"] = YAML::Load(LexicalCast<std::map<std::string, std::string>
				, std::string>()(conf.args));
			for(auto& i : conf.address){
				node["address"].push_back(i);
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	class TcpServer : public std::enable_shared_from_this<TcpServer>
						, Noncopyable{
	public:
		using ptr = std::shared_ptr<TcpServer>;
		TcpServer(Routn::IOManager* worker = Routn::IOManager::GetThis()
				, Routn::IOManager* io_worker = Routn::IOManager::GetThis()
				, Routn::IOManager* accept_worker = Routn::IOManager::GetThis());
		virtual ~TcpServer();

		virtual bool bind(Routn::Address::ptr addr);
		virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fail_addrs);
		virtual bool start();
		virtual void stop();
		
		uint64_t getReadTimeout() const { return _readTimeout;}
		std::string getName() const { return _name;}
		void setReadTimeout(uint64_t v) { _readTimeout = v;} 
		void setName(const std::string& v) { _name = v;}
		void setConf(const TcpServerConf::ptr v) { _conf = v;}
		void setConf(const TcpServerConf& v) { _conf.reset(new TcpServerConf(v));}

		bool isStop() const { return _isStop;}
	protected:
		virtual void handleClient(Socket::ptr client);			//触发回调
		virtual void startAccept(Socket::ptr sock);
	protected:
		std::vector<Socket::ptr> 	_socks;
		IOManager* 				 	_worker;
		IOManager*					_io_worker;
		IOManager*					_accept_worker;
		uint64_t 				 	_readTimeout;
		std::string 			 	_name;
		bool						_isStop;

		TcpServerConf::ptr 			_conf;
	};

}


#endif

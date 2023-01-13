/*************************************************************************
	> File Name: RockStream.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月31日 星期六 22时17分22秒
 ************************************************************************/

#include "RockStream.h"
#include "../Log.h"
#include "../Config.h"
#include "../Worker.h"



namespace Routn{
	static Routn::ConfigVar<std::unordered_map<std::string
		, std::unordered_map<std::string, std::string>> >::ptr g_rock_services = 
		Routn::Config::Lookup("rock_services", std::unordered_map<std::string, std::unordered_map<std::string, std::string>>(), "rock_services");

	std::string RockResult::toString() const {
		std::stringstream ss;
		ss << "[RockResult result=" << result
		<< " resultStr=" << resultStr
		<< " used=" << used
		<< " response=" << (response ? response->toString() : "null")
		<< " request=" << (request ? request->toString() : "null")
		<< "]";
		return ss.str();
	}

	RockStream::RockStream(Socket::ptr sock)
		: AsyncSocketStream(sock, true)
		, _decoder(new RockMessageDecoder){
		ROUTN_LOG_DEBUG(g_logger) << "RockStream::RockStream "
			<< this << " "
			<< (sock ? sock->toString() : " ");
	}

	RockStream::~RockStream(){
		ROUTN_LOG_DEBUG(g_logger) << "RockStream::~RockStream"
			<< this << " "
			<< (_socket ? _socket->toString() : "");
	}

	int32_t RockStream::sendMessage(Message::ptr msg){
		if(isConnected()){
			RockSendCtx::ptr ctx(new RockSendCtx);
			ctx->msg = msg;
			enqueue(ctx);
			return -1;
		}else{
			return -1;
		}
	}

	RockResult::ptr RockStream::request(RockRequest::ptr req, uint32_t timeout_ms){
		if(isConnected()){
			RockCtx::ptr ctx(new RockCtx);
			ctx->request = req;
			ctx->sn = req->getSn();
			ctx->timeout = timeout_ms;
			ctx->scheduler = Schedular::GetThis();
			ctx->fiber = Fiber::GetThis();
			addCtx(ctx);
			uint64_t ts = GetCurrentMs();
			ctx->timer = Routn::IOManager::GetThis()->addTimer(timeout_ms, 
					std::bind(&RockStream::onTimeOut, shared_from_this(), ctx));
			enqueue(ctx);
			Fiber::YieldToHold();
			auto rt = std::make_shared<RockResult>(ctx->result, "ok", Routn::GetCurrentMs() - ts, ctx->response, req);
			rt->server = getSocket()->getRemoteAddress()->toString();
			return rt;
		}else{
			auto rt = std::make_shared<RockResult>(AsyncSocketStream::NOT_CONNECT, "not_connect " + getSocket()->getRemoteAddress()->toString(), 0, nullptr, req);
			rt->server = getSocket()->getRemoteAddress()->toString();
			return rt;
		}
	}


	bool RockStream::RockSendCtx::doSend(AsyncSocketStream::ptr stream){
		return std::dynamic_pointer_cast<RockStream>(stream)->_decoder->serializeTo(stream, msg) > 0;
	}

	bool RockStream::RockCtx::doSend(AsyncSocketStream::ptr stream){
		return std::dynamic_pointer_cast<RockStream>(stream)->_decoder->serializeTo(stream, request) > 0;
	}

	Routn::AsyncSocketStream::Ctx::ptr RockStream::doRecv(){
		auto msg = _decoder->parseFrom(shared_from_this());
		if(!msg){
			innerClose();
			return nullptr;
		}

		int type = msg->getType();
		if(type == Message::RESPONSE){
			auto rsp = std::dynamic_pointer_cast<RockResponse>(msg);
			if(!rsp){
				ROUTN_LOG_WARN(g_logger) << "RockStream do Recv response not RockResponse!" 
					<< msg->toString();
				return nullptr;
			}
			RockCtx::ptr ctx = getAndDelCtxAs<RockCtx>(rsp->getSn());
			if(!ctx){
				ROUTN_LOG_WARN(g_logger) << "RockStream request timeout response"
					<< rsp->toString();
				return nullptr;
			}
			ctx->result = rsp->getResult();
			ctx->response = rsp;
			return ctx;
		}else if(type == Message::REQUEST){
			auto req = std::dynamic_pointer_cast<RockRequest>(msg);
			if(!req){
				ROUTN_LOG_WARN(g_logger) << "RockStream doRecv request is not RockRequest"
					<< msg->toString();
				return nullptr;
			}
			if(_requestHandler){
				_worker->schedule(std::bind(&RockStream::handleRequest,
							std::dynamic_pointer_cast<RockStream>(shared_from_this()),
							req));
			}else{
				ROUTN_LOG_WARN(g_logger) << "unhandle request " << req->toString(); 
			}
		}else if(type == Message::NOTIFY){
			auto ntf = std::dynamic_pointer_cast<RockNotify>(msg);
			if(!ntf){
				ROUTN_LOG_WARN(g_logger) << "RockStream doRecv notify is not RockNotify: "
					<< msg->toString();
				return nullptr;
			}

			if(_notifyHandler){
				_worker->schedule(std::bind(&RockStream::handleNotify, 
							std::dynamic_pointer_cast<RockStream>(shared_from_this()),
							ntf));
			}else{
				ROUTN_LOG_WARN(g_logger) << "unhandle notify " << ntf->toString();
			}
		}else{
			ROUTN_LOG_WARN(g_logger) << "RockStream recv unknown type = " << type 
				<< " msg: " << msg->toString();
		}
		return nullptr;
	}

	static SocketStream::ptr create_rock_stream(const std::string& domain, const std::string& service, ServiceItemInfo::ptr info) {
		IPAddress::ptr addr = Address::LookupAnyIPAddress(info->getIp());
		if(!addr) {
			ROUTN_LOG_ERROR(g_logger) << "invalid service info: " << info->toString();
			return nullptr;
		}
		addr->setPort(info->getPort());

		RockConnection::ptr conn = std::make_shared<RockConnection>();

		WorkerMgr::GetInstance()->schedule("service_io", [conn, addr, domain, service](){
			if(domain == "logserver") {
			 	conn->setConnectCB([conn](AsyncSocketStream::ptr){
			// 		RockRequest::ptr req = std::make_shared<RockRequest>();
			// 		req->setCmd(100);
			// 		logserver::LoginRequest login_req;
			// 		login_req.set_ipport(sylar::Module::GetServiceIPPort("rock"));
			// 		login_req.set_host(sylar::GetHostName());
			// 		login_req.set_pid(getpid());
			// 		req->setAsPB(login_req);
			// 		auto r = conn->request(req, 200);
			// 		if(r->result) {
			// 			SYLAR_LOG_ERROR(g_logger) << "logserver login error: " << r->toString();
			// 			return false;
			// 		}
					return true;
				});
			}
			conn->connect(addr);
			conn->start();
		});
		return conn;
	}

	void RockStream::handleRequest(RockRequest::ptr req){
		Routn::RockResponse::ptr rsp = req->createResponse();
		if(!_requestHandler(req, rsp, std::dynamic_pointer_cast<RockStream>(shared_from_this()))){
			sendMessage(rsp);
			close();
		}else{
			sendMessage(rsp);
		}
	}

	void RockStream::handleNotify(RockNotify::ptr nty){
		if(!_notifyHandler(nty, std::dynamic_pointer_cast<RockStream>(shared_from_this()))){
			close();
		}
	}


	RockSession::RockSession(Socket::ptr sock)
		: RockStream(sock){
		_autoConnect = false;
	}

	RockConnection::RockConnection()
		: RockStream(nullptr){
		_autoConnect = true;
	}

	bool RockConnection::connect(Address::ptr addr){
		_socket = Socket::CreateTCP(addr);
		return _socket->connect(addr);
	}
	

	RockSDLoadBalance::RockSDLoadBalance(IServiceDiscovery::ptr sd)
		: SDLoadBalance(sd){
	}

	void RockSDLoadBalance::start(){
		_callback = create_rock_stream;
		initConf(g_rock_services->getValue());
		SDLoadBalance::start();
	}

	void RockSDLoadBalance::stop(){
		SDLoadBalance::stop();
	}

	void RockSDLoadBalance::start(const std::unordered_map<std::string
				, std::unordered_map<std::string, std::string>>& confs){
		_callback = create_rock_stream;
		initConf(confs);
		SDLoadBalance::start();
	}

	RockResult::ptr RockSDLoadBalance::request(const std::string& domain, const std::string& service,
							RockRequest::ptr req, uint32_t timeout_ms, uint64_t idx){
		auto lb = get(domain, service);
		if(!lb){
			return std::make_shared<RockResult>(ILoadBalance::NO_SERVICE, "no_service", 0, nullptr, req);
		}
		auto conn = lb->get(idx);
		if(!conn){
			return std::make_shared<RockResult>(ILoadBalance::NO_CONNECTION, "no_connection", 0, nullptr, req);
		}
		uint64_t ts = GetCurrentMs();
		auto& stats = conn->get(ts / 1000);
		stats.incDoing(1);
		stats.incTotal(1);
		auto r = conn->getStreamAs<RockStream>()->request(req, timeout_ms);
		uint64_t ts2 = GetCurrentMs();
		if(r->result == 0){
			stats.incOks(1);
			stats.incUsedTime(ts2 - ts);
		}else if(r->result == AsyncSocketStream::TIMEOUT){
			stats.incTimeouts(1);
		}else if(r->result < 0){
			stats.incErrs(1);
		}
		stats.decDoing(1);
		return r;
	}

}
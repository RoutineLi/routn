/*************************************************************************
	> File Name: RockStream.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月31日 星期六 22时15分58秒
 ************************************************************************/

#ifndef _ROCKSTREAM_H
#define _ROCKSTREAM_H


#include "RockProtocol.h"
#include "../streams/AsyncSocketStream.h"
#include "../streams/LoadBalance.h"
#include <boost/any.hpp>

namespace Routn{
	struct RockResult{
		using ptr = std::shared_ptr<RockResult>;
		RockResult(int32_t result, const std::string& resultstr, int32_t used, RockResponse::ptr rsp, RockRequest::ptr req)
			: result(result)
			, resultStr(resultstr)
			, used(used)
			, response(rsp)
			, request(req){

		}
		int32_t result;
		std::string resultStr;
		int32_t used;
		RockResponse::ptr response;
		RockRequest::ptr request;
		std::string server = "";
		std::string toString() const;
	};


	class RockStream : public Routn::AsyncSocketStream{
	public:
		using ptr = std::shared_ptr<RockStream>;
		using request_handler = std::function<bool(RockRequest::ptr
												, RockResponse::ptr
												, RockStream::ptr)>;
		using notify_handler = std::function<bool(RockNotify::ptr, RockStream::ptr)>;
		RockStream(Socket::ptr sock);
		~RockStream();

		int32_t sendMessage(Message::ptr msg);
		RockResult::ptr request(RockRequest::ptr req, uint32_t timeout_ms);

		request_handler getRequestHandler() const { return _requestHandler;}
		notify_handler getNotifyHandler() const { return _notifyHandler;}

		void setRequestHandler(request_handler v) { _requestHandler = v;}
		void setNotifyHandler(notify_handler v) { _notifyHandler = v;}

		template<class T>
		void setData(const T& t){
			_data = t;
		}

		template<class T>
		T getData(){
			try{
				return boost::any_cast<T>(_data);
			}catch(...){
			}
			return T();
		}
	protected:
		struct RockSendCtx : public SendCtx{
			using ptr = std::shared_ptr<RockSendCtx>;
			Message::ptr msg;
			virtual bool doSend(AsyncSocketStream::ptr stream) override;
		};
		struct RockCtx : public Ctx{
			using ptr = std::shared_ptr<RockCtx>;
			RockRequest::ptr request;
			RockResponse::ptr response;

			virtual bool doSend(AsyncSocketStream::ptr stream) override;
		};

		virtual Ctx::ptr doRecv() override;
		void handleRequest(RockRequest::ptr req);
		void handleNotify(RockNotify::ptr nty);
	private:
		RockMessageDecoder::ptr _decoder;
		request_handler _requestHandler;
		notify_handler _notifyHandler;
		boost::any _data;
	};

	class RockSession : public RockStream{
	public:
		using ptr = std::shared_ptr<RockSession>;
		RockSession(Socket::ptr sock);
	};

	class RockConnection : public RockStream{
	public:
		using ptr = std::shared_ptr<RockConnection>;
		RockConnection();
		bool connect(Routn::Address::ptr addr);
	};

	class RockSDLoadBalance : public SDLoadBalance{
	public:
		using ptr = std::shared_ptr<RockSDLoadBalance>;
		RockSDLoadBalance(IServiceDiscovery::ptr sd);

		virtual void start();
		virtual void stop();
		void start(const std::unordered_map<std::string
					, std::unordered_map<std::string, std::string>>& confs);
		RockResult::ptr request(const std::string& domain, const std::string& service,
								RockRequest::ptr req, uint32_t timeout_ms, uint64_t idx = -1);
	};
}


#endif

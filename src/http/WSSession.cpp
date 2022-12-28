/*************************************************************************
	> File Name: WSSession.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月10日 星期六 00时42分50秒
 ************************************************************************/

#include "WSSession.h"
#include "../Log.h"
#include "../Endian.h"
#include "../Util.h"

#include <cstring>

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

Routn::ConfigVar<uint32_t>::ptr g_websocket_message_max_size
    = Routn::Config::Lookup("websocket.message.max_size"
            ,(uint32_t) 1024 * 1024 * 32, "websocket message max size");
namespace Routn{
namespace Http{
	WSSession::WSSession(Socket::ptr sock, bool owner)
		: HttpSession(sock, owner){

	}

	HttpRequest::ptr WSSession::handleShake(){
		HttpRequest::ptr req;
		do{
			req = recvRequest();
			if(!req){
				ROUTN_LOG_ERROR(g_logger) << "invalid http request";
				break;
			}

			if(strcasecmp(req->getHeader("Upgrade").c_str(), "websocket")){
				ROUTN_LOG_ERROR(g_logger) << "http header Upgrade is not websocket";
				break;
			}
			if(req->getHeader("Connection").find("Upgrade") == req->getHeader("Connection").npos){
				ROUTN_LOG_ERROR(g_logger) << "http header Connection is not Upgrade";
				break;
			}
			if(req->getHeaderAs<int>("Sec-webSocket-Version") != 13){
				ROUTN_LOG_ERROR(g_logger) << "http header Sec-webSocket-Version not supported";
				break;
			}
			std::string key = req->getHeader("Sec-WebSocket-key");
			if(key.empty()){
				ROUTN_LOG_ERROR(g_logger) << "http header Sec-WebSocket-Key is null";
				break;
			}
			
			std::string v = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
			v = Routn::base64encode(Routn::sha1sum(v));
			req->setWebsocket(true);

			auto rsp = std::make_shared<HttpResponse>(req->getVersion()
						, req->isClose());
			
			rsp->setStatus(HttpStatus::SWITCHING_PROTOCOLS);
			rsp->setWebsocket(true);
			rsp->setReason("Web Socket Protocol Handshake");
			rsp->setHeader("Upgrade", "websocket");
			rsp->setHeader("Connection", "Upgrade");
			rsp->setHeader("Sec-WebSocket-Accept", v);
			rsp->setHeader("Sec-WebSocket-Version", "13");

			sendResponse(rsp);
			ROUTN_LOG_DEBUG(g_logger) << req->toString();
			ROUTN_LOG_DEBUG(g_logger) << rsp->toString();
			return req;
		}while(false);
		if(req){
			ROUTN_LOG_INFO(g_logger) << req->toString();
		}
		return nullptr;
	}

	WSFrameMessage::WSFrameMessage(int opcode, const std::string& data)
		: _opcode(opcode)
		, _data(data){

	}

	std::string WSFrameHead::toString() const {
		std::stringstream ss;
		ss << "[WSFrameHead fin=" << fin
		<< " rsv1=" << rsv1
		<< " rsv2=" << rsv2
		<< " rsv3=" << rsv3
		<< " opcode=" << opcode
		<< " mask=" << mask
		<< " payload=" << payload
		<< "]";
		return ss.str();
	}

	WSFrameMessage::ptr WSSession::recvMessage(){
		return WSRecvMessage(this, false);
	}

	int32_t WSSession::sendMessage(WSFrameMessage::ptr msg, bool fin){
		return WSSendMessage(this, msg, false, fin);
	}

	int32_t WSSession::sendMessage(const std::string& msg, int32_t opcode, bool fin){
		return WSSendMessage(this, std::make_shared<WSFrameMessage>(opcode, msg), false, fin);
	}

	int32_t WSSession::ping(){
		return WSPing(this);
	}

	int32_t WSSession::pong(){
		return WSPong(this);
	}

	WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool client){
		int opcode = 0;
		std::string data;
		int cur_len = 0;
		do{
			WSFrameHead ws_head;
			//stream流读取协议头帧
			if(stream->readFixSize(&ws_head, sizeof(ws_head)) <= 0){
				break;
			}
			ROUTN_LOG_INFO(g_logger) << "WSFreamHead " << ws_head.toString();

			if(ws_head.opcode == WSFrameHead::PING){
				ROUTN_LOG_INFO(g_logger) << "PING";
				if(WSPong(stream) <= 0){
					break;
				}
			}else if(ws_head.opcode == WSFrameHead::PONG){
			}else if(ws_head.opcode == WSFrameHead::CONTINUE
					|| ws_head.opcode == WSFrameHead::TEXT_FRAME
					|| ws_head.opcode == WSFrameHead::BIN_FRAME){
				if(!client && !ws_head.mask){
					ROUTN_LOG_INFO(g_logger) << "WSFrameHead mask = " << ws_head.mask;
					break;
				}
				uint64_t length = 0;
				if(ws_head.payload == 126){//payload长度没有超过7位
					uint16_t len = 0;
					if(stream->readFixSize(&len, sizeof(len)) <= 0){
						break;
					}
					length = Routn::byteswapOnLittleEndian(len);
				}else if(ws_head.payload == 127){
					uint64_t len = 0;
					if(stream->readFixSize(&len, sizeof(len)) <= 0){
						break;
					}
					length = Routn::byteswapOnLittleEndian(len);
				}else{
					length = ws_head.payload;
				}

				if((cur_len + length) >= g_websocket_message_max_size->getValue()){
					ROUTN_LOG_WARN(g_logger) << "WSFrameMessage length > "
						<< g_websocket_message_max_size->getValue()
						<< " (" << (cur_len + length) << ")";
					break;
				}

				char mask[4] = {0};
				if(ws_head.mask){
					if(stream->readFixSize(mask, sizeof(mask)) <= 0){
						break;
					}
				}
				data.resize(cur_len + length);
				//recv websocket-dataframe
				if(stream->readFixSize(&data[cur_len], length) <= 0){
					break;
				}
				if(ws_head.mask){
					for(int i = 0; i < (int)length; ++i){
						//decode data
						data[cur_len + i] ^= mask[i % 4];
					}
				}
				cur_len += length;

				if(!opcode && ws_head.opcode != WSFrameHead::CONTINUE){
					opcode = ws_head.opcode;
				}

				if(ws_head.fin){
					ROUTN_LOG_DEBUG(g_logger) << data;
					return std::make_shared<WSFrameMessage>(opcode, std::move(data));
				}
			}else{
				ROUTN_LOG_ERROR(g_logger) << "invalid opcode = " << ws_head.opcode;
			}
		}while(true);
		stream->close();
		return nullptr;
	}

	int32_t WSSendMessage(Stream* stream, WSFrameMessage::ptr msg, bool client, bool fin){
		do{
			WSFrameHead ws_head;
			memset(&ws_head, 0, sizeof(ws_head));
			ws_head.fin = fin;
			ws_head.opcode = msg->getOpcode();
			ws_head.mask = client;
			uint64_t size = msg->getData().size();
			if(size < 126){
				ws_head.payload = size;
			}else if(size < 65536){
				ws_head.payload = 126;
			}else{
				ws_head.payload = 127;
			}

			if(stream->writeFixSize(&ws_head, sizeof(ws_head)) <= 0)
				break;
			if(ws_head.payload == 126){
				uint16_t len = size;
				len = Routn::byteswapOnLittleEndian(len);
				if(stream->writeFixSize(&len, sizeof(len)) <= 0){
					break;
				}
			}else if(ws_head.payload == 127){
				uint64_t len = Routn::byteswapOnLittleEndian(len);
				if(stream->writeFixSize(&len, sizeof(len)) <= 0){
					break;
				}
			}
			if(client){
				char mask[4];
				uint32_t rand_val = rand();
				memcpy(mask, &rand_val, sizeof(mask));
				std::string& data = msg->getData();
				for(size_t i = 0; i < data.size(); ++i){
					data[i] ^= mask[i % 4];
				}

				if(stream->writeFixSize(mask, sizeof(mask)) <= 0){
					break;
				}
			}
			if(stream->writeFixSize(msg->getData().c_str(), size) <= 0){
				break;
			}
			return size + sizeof(ws_head);
		}while(0);
		stream->close();
		return -1;
	}

	int32_t WSPing(Stream* stream){
		WSFrameHead ws_head;
		memset(&ws_head, 0, sizeof(ws_head));
		ws_head.fin = 1;
		ws_head.opcode = WSFrameHead::PING;
		int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
		if(v <= 0){
			stream->close();
		}
		return v;
	}

	int32_t WSPong(Stream* stream){
		WSFrameHead ws_head;
		memset(&ws_head, 0, sizeof(ws_head));
		ws_head.fin = 1;
		ws_head.opcode = WSFrameHead::PONG;
		int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
		if(v <= 0){
			stream->close();
		}
		return v;
	}

}
}


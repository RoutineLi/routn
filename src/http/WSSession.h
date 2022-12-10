/*************************************************************************
	> File Name: WSSession.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月10日 星期六 00时42分46秒
 ************************************************************************/

#ifndef _WSSESSION_H
#define _WSSESSION_H

#include "../Config.h"
#include "HttpSession.h"
#include <cstdint>

namespace Routn{
namespace Http{

#pragma pack(1)

/**
 * @brief WebSocket协议帧头部
 * 
 */
	struct WSFrameHead{
		enum OPCODE{
			/// 数据分片帧
			CONTINUE =  0,
			/// 文本帧
			TEXT_FRAME = 1,
			/// 二进制帧
			BIN_FRAME = 2,
			/// 断开连接
			CLOSE = 8,
			/// PING
			PING = 0x9,
			/// PONG
			PONG = 0xA
		};
		uint32_t opcode: 4;			//4位opcode
		bool rsv3: 1;					//1位rsv3
		bool rsv2: 1;					//1位rsv2
		bool rsv1: 1;					//1位rsv1
		bool fin: 1;					//1位FIN标志位
		uint32_t payload: 7;			//7位Payload
		bool mask: 1;					//1位mask掩码

		std::string toString() const ;

	};
#pragma pack()


	/**
	 * @brief WebSocket协议帧消息体
	 * 
	 */
	class WSFrameMessage{
	public:
		using ptr = std::shared_ptr<WSFrameMessage>;
		WSFrameMessage(int opcode = 0, const std::string& data = "");

		int getOpcode() const { return _opcode;}
		void setOpcode(int v) { _opcode = v;}

		const std::string& getData() const { return _data;}
		std::string& getData() { return _data;}
		void setData(const std::string& v) { _data = v;}
	private:
		int _opcode;
		std::string _data;
	};

	/**
	 * @brief WebSocket会话
	 * 
	 */
	class WSSession : public HttpSession{
	public:
		using ptr = std::shared_ptr<WSSession>;
		WSSession(Socket::ptr sock, bool owner = true);

		HttpRequest::ptr handleShake();

		WSFrameMessage::ptr recvMessage();
		int32_t sendMessage(WSFrameMessage::ptr msg, bool fin = true);
		int32_t sendMessage(const std::string& msg, int32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);
		int32_t ping();
		int32_t pong();
	private:
		bool handleServerShake();
		bool handleClientShake();
	};

	WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool client);
	int32_t WSSendMessage(Stream* stream, WSFrameMessage::ptr msg, bool client, bool fin);
	int32_t WSPing(Stream* stream);
	int32_t WSPong(Stream* stream);
}
}


#endif

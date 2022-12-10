/*************************************************************************
	> File Name: Protocol.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月09日 星期五 23时47分12秒
 ************************************************************************/

#ifndef _PROTOCOL_H
#define _PROTOCOL_H


#include <memory>
#include "Stream.h"
#include "ByteArray.h"

/**
 * @brief 自定义协议，底层是基于zigzag编码的ByteArray序列化
 * 
 */

namespace Routn{

	/**
	 * @brief Message基类
	 * 
	 */
	class Message{
	public:
		using ptr = std::shared_ptr<Message>;
		enum MessageType{
			REQUEST = 1,
			RESPONSE = 2,
			NOTIFY = 3
		};
		virtual ~Message(){}
		virtual ByteArray::ptr toByteArray();
		virtual bool serializeToByteArray(ByteArray::ptr bytearray) = 0;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray) = 0;

		virtual std::string toString() const = 0;
		virtual const std::string& getName() const = 0;
		virtual int32_t getType() const = 0;
	};


	/**
	 * @brief Message解码基类
	 * 
	 */
	class MessageDecoder{
	public:
		using ptr = std::shared_ptr<MessageDecoder>;

		virtual ~MessageDecoder(){}
		virtual Message::ptr parseFrom(Stream::ptr stream) = 0;					//从流接口解析成Message
		virtual int32_t serializeTo(Stream::ptr stream, Message::ptr msg) = 0;	//将Message序列化为流
	};

	/**
	 * @brief Request-Message
	 * 
	 */
	class Request : public Message{
	public:
		using ptr = std::shared_ptr<Request>;

		Request();

		uint32_t getSn() const { return _sn;}
		uint32_t getCmd() const { return _cmd;}

		void setSn(uint32_t v) { _sn = v;}
		void setCmd(uint32_t v) { _cmd = v;}

		virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
	protected:
		uint32_t _sn;
		uint32_t _cmd;
	};

	/**
	 * @brief Response-Message
	 * 
	 */
	class Response : public Message{
	public:
		using ptr = std::shared_ptr<Response>;
		Response();

		uint32_t getSn() const { return _sn;}
		uint32_t getCmd() const { return _cmd;}
		uint32_t getResult() const { return _result;}
		const std::string& getResStr() const { return _resStr;} 

		void setSn(uint32_t v) { _sn = v;}
		void setCmd(uint32_t v) { _cmd = v;}
		void setResult(uint32_t v) { _result = v;}
		void setResStr(const std::string& v) { _resStr = v;}

		virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
	protected:
		uint32_t _sn;
		uint32_t _cmd;
		uint32_t _result;
		std::string _resStr;
	};

	/**
	 * @brief Notify-Message
	 * 
	 */
	class Notify : public Message{
	public:
		using ptr = std::shared_ptr<Notify>;
		Notify();

		uint32_t getNotify() const { return _notify;}
		void setNotify(uint32_t v) { _notify = v;}
		
		virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
	protected:
		uint32_t _notify;
	};
}


#endif

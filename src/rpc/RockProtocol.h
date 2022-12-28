/*************************************************************************
	> File Name: RockProtocol.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月28日 星期三 22时48分39秒
 ************************************************************************/

#ifndef _ROCKPROTOCOL_H
#define _ROCKPROTOCOL_H

#include "../Protocol.h"
#include <google/protobuf/message.h>


/**
 * @brief 自定义分布式协议
 * 
 */
namespace Routn{

	/// @brief 协议头
	struct RockMsgHeader{
		RockMsgHeader();
		uint8_t magic[2];	//魔法数
		uint8_t version;	//版本号
		uint8_t flag;		//flag位
		int32_t length;		//长度
	};

	/// @brief 协议消息体 
	class RockBody{
	public:
		using ptr = std::shared_ptr<RockBody>;
		virtual ~RockBody(){}

		void setBody(const std::string& v) { _body = v;}
		const std::string& getBody() const { return _body;}

		virtual bool serializeToByteArray(ByteArray::ptr bytearray);
		virtual bool parseFromByteArray(ByteArray::ptr bytearray);

		template<class T>
		std::shared_ptr<T> getAsPB() const {
			try{
				std::shared_ptr<T> data = std::make_shared<T>();
				if(data->ParseFromString(_body)){
					return data;
				}
			}catch(...){
			}
			return nullptr;
		}

		template<class T>
		bool setAsPB(const T& v){
			try{
				return v.SerializeToString(&_body);
			}catch(...){
			}
			return false;
		}
	protected:
		std::string _body;
	};


	class RockResponse : public Response, public RockBody{
	public:
		using ptr = std::shared_ptr<RockResponse>;

		virtual std::string toString() const override;
		virtual const std::string& getName() const override;
		virtual int32_t getType() const override;

		virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
	};

	class RockRequest : public Request, public RockBody{
	public:
		using ptr = std::shared_ptr<RockRequest>;

		RockResponse::ptr createResponse();

		virtual std::string toString() const override;
		virtual const std::string& getName() const override;
		virtual int32_t getType() const override;

		virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
	};

	class RockNotify : public Notify, public RockBody {
	public:
		typedef std::shared_ptr<RockNotify> ptr;

		virtual std::string toString() const override;
		virtual const std::string& getName() const override;
		virtual int32_t getType() const override;

		virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
	};

	/// @brief 解析器
	class RockMessageDecoder : public MessageDecoder{
	public:
		using ptr = std::shared_ptr<RockMessageDecoder>;

		virtual Message::ptr parseFrom(Stream::ptr stream) override;
		virtual int32_t serializeTo(Stream::ptr stream, Message::ptr msg) override;
	};
}


#endif

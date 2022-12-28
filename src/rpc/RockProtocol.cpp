/*************************************************************************
	> File Name: RockProtocol.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月28日 星期三 22时48分53秒
 ************************************************************************/

#include "RockProtocol.h"
#include "../Log.h"
#include "../Config.h"
#include "../Endian.h"
#include "../streams/ZLIBStream.h"
#include "../Macro.h"

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn{

	static Routn::ConfigVar<uint32_t>::ptr g_rock_protocol_max_length 
		= Config::Lookup("rock.protocol.max_length", (uint32_t)(1024 * 1024 * 64), "rock protocol max len");

	static Routn::ConfigVar<uint32_t>::ptr g_rock_protocol_gzip_min_length
		= Config::Lookup("rock.protocol.gzip_min_length", (uint32_t)(1024 * 1024 * 64), "rock protocol gzip min len");

	bool RockBody::serializeToByteArray(ByteArray::ptr bytearray){
		bytearray->writeStringF32(_body);
		return true;
	}

	bool RockBody::parseFromByteArray(ByteArray::ptr bytearray){
		_body = bytearray->readStringF32();
		return true;
	}



	RockResponse::ptr RockRequest::createResponse(){
		RockResponse::ptr rt = std::make_shared<RockResponse>();
		rt->setSn(_sn);
		rt->setCmd(_cmd);
		return rt;
	}

	std::string RockRequest::toString() const {
		std::stringstream ss;
		ss << "[RockRequest sn = " << _sn
		   << " cmd = " << _cmd
		   << " bodylen = " << _body.size()
		   << "]";
		return ss.str();
	}

	const std::string& RockRequest::getName() const {
		static const std::string& s_name = "RockRequest";
		return s_name;
	}

	int32_t RockRequest::getType() const {
		return Message::REQUEST;
	}

	bool RockRequest::serializeToByteArray(ByteArray::ptr bytearray){
		try{
			bool v = true;
			v &= Request::serializeToByteArray(bytearray);
			v &= RockBody::serializeToByteArray(bytearray);
			return v;
		}catch(...){
			ROUTN_LOG_ERROR(g_logger) << "RockRequest serializeToByteArray error"; 
		}
		return false;
	}

	bool RockRequest::parseFromByteArray(ByteArray::ptr bytearray){
		try{
			bool v = true;
			v &= Request::parseFromByteArray(bytearray);
			v &= RockBody::parseFromByteArray(bytearray);
			return v;
		}catch(...){
			ROUTN_LOG_ERROR(g_logger) << "RockRequest parseFromByteArray error"
				<< bytearray->toHexString(); 
		}
		return false;
	}




	std::string RockResponse::toString() const {
		std::stringstream ss;
		ss << "[RockResponse sn = " << _sn
		   << " cmd = " << _cmd
		   << " result = " << _result
		   << " result_msg = " << _resStr
		   << " bodylen = " << _body.size()
		   << "]";
		return ss.str();
	}

	const std::string& RockResponse::getName() const {
		static const std::string& s_name = "RockResponse";
		return s_name;
	}

	int32_t RockResponse::getType() const {
		return Message::RESPONSE;
	}

	bool RockResponse::serializeToByteArray(ByteArray::ptr bytearray){
		try {
			bool v = true;
			v &= Response::serializeToByteArray(bytearray);
			v &= RockBody::serializeToByteArray(bytearray);
			return v;
		} catch (...) {
			ROUTN_LOG_ERROR(g_logger) << "RockResponse serializeToByteArray error";
		}
		return false;
	}

	bool RockResponse::parseFromByteArray(ByteArray::ptr bytearray){
		try {
			bool v = true;
			v &= Response::parseFromByteArray(bytearray);
			v &= RockBody::parseFromByteArray(bytearray);
			return v;
		} catch (...) {
			ROUTN_LOG_ERROR(g_logger) << "RockResponse parseFromByteArray error";
		}
		return false;
	}




	
	std::string RockNotify::toString() const {
		std::stringstream ss;
		ss << "[RockNotify notify = " << _notify
		   << " bodylen = " << _body.size()
		   << "]";
		return ss.str();
	}

	const std::string& RockNotify::getName() const {
		static const std::string& s_name = "RockNotify";
		return s_name;
	}

	int32_t RockNotify::getType() const {
		return Message::NOTIFY;
	}

	bool RockNotify::serializeToByteArray(ByteArray::ptr bytearray){
		try{
			bool v = true;
			v &= Notify::serializeToByteArray(bytearray);
			v &= RockBody::serializeToByteArray(bytearray);
			return v;
		}catch(...){
			ROUTN_LOG_ERROR(g_logger) << "RockNotify serializeToByteArray error";
		}
		return false;
	}

	bool RockNotify::parseFromByteArray(ByteArray::ptr bytearray){
		try {
			bool v = true;
			v &= Notify::parseFromByteArray(bytearray);
			v &= RockBody::parseFromByteArray(bytearray);
			return v;
		} catch (...) {
			ROUTN_LOG_ERROR(g_logger) << "RockNotify parseFromByteArray error";
		}
		return false;
	}

	static constexpr uint8_t s_rock_magic[2] = {0x12, 0x21};

	RockMsgHeader::RockMsgHeader()
		:magic{s_rock_magic[0], s_rock_magic[1]}
		,version(1)
		,flag(0)
		,length(0) {
	}

	Message::ptr RockMessageDecoder::parseFrom(Stream::ptr stream){
		try{
			RockMsgHeader header;
			int rt = stream->readFixSize(&header, sizeof(header));
			if(rt <= 0){
				ROUTN_LOG_DEBUG(g_logger) << "RockMessageDecoder decode head error rt = " << rt;
				return nullptr;
			}

			if(memcmp(header.magic, s_rock_magic, sizeof(s_rock_magic))){
				ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder head.magic error";
				return nullptr;
			}

			if(header.version != 0x1){
				ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder head.verison is not 1.0";
				return nullptr;
			}

			header.length = Routn::byteswapOnLittleEndian(header.length);
			if((uint32_t)header.length >= g_rock_protocol_max_length->getValue()){
				ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder head.length("
										  << header.length << ") >= "
										  << g_rock_protocol_max_length->getValue();
				return nullptr;
			}
			Routn::ByteArray::ptr ba = std::make_shared<ByteArray>();
			rt = stream->readFixSize(ba, header.length);
			if(rt <= 0){
				ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder read body fail len = " << rt << " errno = " << errno
					<< " - " << strerror(errno);
				return nullptr;
			}

			ba->setPosition(0);
			//标志位置为1表示gzip压缩
			if(header.flag & 0x1){
				auto zstream = ZLIBStream::CreateGzip(false);
				if(zstream->write(ba, -1) != Z_OK){
					ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder unzip error";
					return nullptr;
				}
				if(zstream->flush() != Z_OK){
					ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder unzip flush error";
					return nullptr;
				}
				ba = zstream->getByteArray();
			}
			uint8_t type = ba->readFuint8();
			Message::ptr msg;
			switch (type)
			{
			case Message::REQUEST:
				msg = std::make_shared<RockRequest>();
				break;
			
			case Message::RESPONSE:
				msg = std::make_shared<RockResponse>();
				break;

			case Message::NOTIFY:
				msg = std::make_shared<RockNotify>();
				break;
			default:
				ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder invalid msg type " << type;
				return nullptr;
			}

			if(!msg->parseFromByteArray(ba)){
				ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder parseFromByteArray fail";
				return nullptr;
			}
			return msg;
		}catch(std::exception& e){
			ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder except: " << e.what();
		}catch(...){
			ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder except";
		}
		return nullptr;
	}

	int32_t RockMessageDecoder::serializeTo(Stream::ptr stream, Message::ptr msg){
		RockMsgHeader header;
		auto ba = msg->toByteArray();
		ba->setPosition(0);
		header.length = ba->getSize();
		if((uint32_t)header.length >= g_rock_protocol_gzip_min_length->getValue()){
			auto zstream = Routn::ZLIBStream::CreateGzip(true);
			if(zstream->write(ba, -1) != Z_OK){
				ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder serializeTo gzip error";
				return -1;
			}
			if(zstream->flush() != Z_OK){
				ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder serializeTo gzip flush error";
				return -2;
			}
			ba = zstream->getByteArray();
			header.flag = 0x1;
			header.length = ba->getSize();
		}
		header.length = Routn::byteswapOnLittleEndian(header.length);
		int rt = stream->writeFixSize(&header, sizeof(header));
		if(rt <= 0){
			ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder serializeTo write header len = " 
				<< rt << " errno = " << errno << " - " << strerror(errno);
			return -3;
		}
		rt = stream->writeFixSize(ba, ba->getReadSize());
		if(rt <= 0){
			ROUTN_LOG_ERROR(g_logger) << "RockMessageDecoder serializeTo write body fail rt=" << rt
				<< " errno=" << errno << " - " << strerror(errno);
			return -4;
		}
		return sizeof(header) + ba->getSize();
	}

}


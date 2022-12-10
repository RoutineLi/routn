/*************************************************************************
	> File Name: Protocol.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月09日 星期五 23时47分22秒
 ************************************************************************/

#include "Protocol.h"
#include "Util.h"

namespace Routn{

	ByteArray::ptr Message::toByteArray(){
		ByteArray::ptr ba(new ByteArray);
		if(serializeToByteArray(ba)){
			return ba;
		}
		return nullptr;
	}

	Request::Request()
		: _sn(0)
		, _cmd(0){

	}


	bool Request::serializeToByteArray(ByteArray::ptr bytearray){
		bytearray->writeFuint8(getType());
		bytearray->writeUint32(_sn);
		bytearray->writeUint32(_cmd);
		return true;
	}

	bool Request::parseFromByteArray(ByteArray::ptr bytearray){
		_sn = bytearray->readUint32();
		_cmd = bytearray->readUint32();
		return true;
	}

	Response::Response()
		: _sn(0)
		, _cmd(0)
		, _result(404)
		, _resStr("unavaliable"){

	}

	bool Response::serializeToByteArray(ByteArray::ptr bytearray){
		bytearray->writeFuint8(getType());
		bytearray->writeUint32(_sn);
		bytearray->writeUint32(_cmd);
		bytearray->writeUint32(_result);
		bytearray->writeStringVint(_resStr);
		return true;
	}

	bool Response::parseFromByteArray(ByteArray::ptr bytearray){
		_sn = bytearray->readUint32();
		_cmd = bytearray->readUint32();
		_result = bytearray->readUint32();
		_resStr = bytearray->readStringVint();
		return true;
	}

	Notify::Notify()
		: _notify(0){

	}

	bool Notify::serializeToByteArray(ByteArray::ptr bytearray){
		bytearray->writeFuint8(getType());
		bytearray->writeUint32(_notify);
		return true;
	}

	bool Notify::parseFromByteArray(ByteArray::ptr bytearray){
		_notify = bytearray->readUint32();
		return true;
	}
}


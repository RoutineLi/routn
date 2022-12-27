/*************************************************************************
	> File Name: chat_protocol.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月18日 星期日 14时57分16秒
 ************************************************************************/

#include "chat_protocol.h"
#include "src/JsonUtil.h"


using namespace configor;

namespace Chat{

	ChatMessage::ptr ChatMessage::Create(const std::string& v){
		json::value json;
		if(!Routn::JsonUtil::FromString(json, v)){
			return nullptr;
		}
		ChatMessage::ptr rt(new ChatMessage);
		for(auto iter = json.begin(); iter != json.end(); ++iter){
			rt->_datas[iter.key()] = iter.value().get<std::string>();
		}
		return rt;
	}

	ChatMessage::ChatMessage(){

	}

	std::string ChatMessage::get(const std::string& name){
		auto it = _datas.find(name);
		return it == _datas.end() ? "" : it->second;
	}

	void ChatMessage::set(const std::string& name, const std::string& val){
		_datas[name] = val;
	}

	std::string ChatMessage::toString() const {
		json::value json;
		for(auto &i : _datas){
			json[i.first] = i.second;
		}
		return Routn::JsonUtil::ToString(json);
	}

}


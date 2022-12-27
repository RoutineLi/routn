/*************************************************************************
	> File Name: JsonUtil.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月18日 星期日 15时12分56秒
 ************************************************************************/

#include "JsonUtil.h"

namespace Routn{

	bool JsonUtil::NeedEscape(const std::string& v) {
		for(auto& c : v) {
			switch(c) {
				case '\f':
				case '\t':
				case '\r':
				case '\n':
				case '\b':
				case '"':
				case '\\':
					return true;
				default:
					break;
			}
		}
		return false;
	}

	std::string JsonUtil::Escape(const std::string& v) {
		size_t size = 0;
		for(auto& c : v) {
			switch(c) {
				case '\f':
				case '\t':
				case '\r':
				case '\n':
				case '\b':
				case '"':
				case '\\':
					size += 2;
					break;
				default:
					size += 1;
					break;
			}
		}
		if(size == v.size()) {
			return v;
		}

		std::string rt;
		rt.resize(size);
		for(auto& c : v) {
			switch(c) {
				case '\f':
					rt.append("\\f");
					break;
				case '\t':
					rt.append("\\t");
					break;
				case '\r':
					rt.append("\\r");
					break;
				case '\n':
					rt.append("\\n");
					break;
				case '\b':
					rt.append("\\b");
					break;
				case '"':
					rt.append("\\\"");
					break;
				case '\\':
					rt.append("\\\\");
					break;
				default:
					rt.append(1, c);
					break;

			}
		}
		return rt;
	}


	std::string JsonUtil::GetStringValue(const json::value& json
										, const std::string& default_value){
		if(json.is_string()){
			return json.get<std::string>();
		}else if(json.is_array()){
			return ToString(json);
		}else if(json.is_object()){
			return ToString(json);
		}else{
			return json.get<std::string>();
		}
		return default_value;
	}

	double JsonUtil::GetDoubleValue(const json::value& json
									, double default_value){
        if(json.is_floating()){
			return json.get<double>();
		}else if(json.is_string()){
			return atof(json::dump(json).c_str());
		}
		return default_value;
	}

	int32_t JsonUtil::GetInt32Value(const json::value& json
									, int32_t default_value){
        if(json.is_integer()){
			return json.get<int32_t>();
		}else if(json.is_string()){
			return atoi(json::dump(json).c_str());
		}
		return default_value;
	}

	uint32_t JsonUtil::GetUint32Value(const json::value& json
									, uint32_t default_value){
        if(json.is_number()){
			return json.get<uint32_t>();
		}else if(json.is_string()){
			return atoi(json::dump(json).c_str());
		}
		return default_value;
	}

	int64_t JsonUtil::GetInt64Value(const json::value& json
									, int64_t default_value){
        if(json.is_integer()){
			return json.get<int64_t>();
		}else if(json.is_string()){
			return atoi(json::dump(json).c_str());
		}
		return default_value;
	}

	uint64_t JsonUtil::GetUint64Value(const json::value& json
									, uint64_t default_value){
        if(json.is_number()){
			return json.get<uint64_t>();
		}else if(json.is_string()){
			return atoi(json::dump(json).c_str());
		}
		return default_value;
	}	


	std::string JsonUtil::GetString(const json::value& json
									,const std::string& name
                      				,const std::string& default_value){
        if(json[name].is_null()){
			return default_value;
		}
		return GetStringValue(json[name], default_value);
	}


	int32_t JsonUtil::GetInt32(const json::value& json
					,const std::string& name
					,int32_t default_value) {
		if(json[name].is_null()) {
			return default_value;
		}
		return GetInt32Value(json[name], default_value);
	}

	uint32_t JsonUtil::GetUint32(const json::value& json
					,const std::string& name
					,uint32_t default_value) {
		if(json[name].is_null()) {
			return default_value;
		}
		return GetUint32Value(json[name], default_value);
	}

	int64_t JsonUtil::GetInt64(const json::value& json
					,const std::string& name
					,int64_t default_value) {
		if(json[name].is_null()) {
			return default_value;
		}
		return GetInt64Value(json[name], default_value);
	}

	uint64_t JsonUtil::GetUint64(const json::value& json
					,const std::string& name
					,uint64_t default_value) {
		if(json[name].is_null()) {
			return default_value;
		}
		return GetUint64Value(json[name], default_value);
	}

	bool JsonUtil::FromString(json::value& json, const std::string& v){
		json = json::parse(v);
		return !json.is_null();
	}

	std::string JsonUtil::ToString(const json::value& json, bool emit_utf8){
		return json::dump(json);
	}
}


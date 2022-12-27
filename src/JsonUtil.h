/*************************************************************************
	> File Name: JsonUtil.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月18日 星期日 15时12分52秒
 ************************************************************************/

#ifndef _JSONUTIL_H
#define _JSONUTIL_H

#include <string>
#include <iostream>

#include "plugins/configor/json.hpp"

using namespace configor;

namespace Routn{

	struct JsonUtil{
		static bool NeedEscape(const std::string& v);
		static std::string Escape(const std::string& v);

		static std::string GetString(const json::value& json
									, const std::string& name
									, const std::string& default_value = "");
		static double GetDouble(const json::value& json
								, const std::string& name
								, double default_value = 0);
		static int32_t GetInt32(const json::value& json
								, const std::string& name
								, int32_t default_value = 0);
		static uint32_t GetUint32(const json::value& json
								, const std::string& name
								, uint32_t default_value = 0);
		static int64_t GetInt64(const json::value& json
								, const std::string& name
								, int64_t default_value = 0);		
		static uint64_t GetUint64(const json::value& json
								, const std::string& name
								, uint64_t default_value = 0);
		static std::string GetStringValue(const json::value& json
								, const std::string& default_value = "");
		static double GetDoubleValue(const json::value& json
								, double default_value = 0);
		static int32_t GetInt32Value(const json::value& json
								, int32_t default_value = 0);
		static uint32_t GetUint32Value(const json::value& json
								, uint32_t default_value = 0);
		static int64_t GetInt64Value(const json::value& json
								, int64_t default_value = 0);
		static uint64_t GetUint64Value(const json::value& json
								, uint64_t default_value = 0);		
	
		static bool FromString(json::value& json, const std::string& v);
		static std::string ToString(const json::value& json, bool emit_utf8 = true);
	};
}
#endif

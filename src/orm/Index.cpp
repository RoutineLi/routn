/*************************************************************************
	> File Name: Index.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月23日 星期一 12时36分58秒
 ************************************************************************/

#include "Index.h"
#include "../Log.h"
#include "../Util.h"

namespace Routn{
namespace Orm{

	static Logger::ptr g_logger = ROUTN_LOG_NAME("orm");

	bool Index::init(const TiXmlElement& node){
		if(!node.Attribute("name")){
			ROUTN_LOG_ERROR(g_logger) << "index name not exists";
			return false;
		}
		_name = node.Attribute("name");

		if(!node.Attribute("type")){
			ROUTN_LOG_ERROR(g_logger) << "index name = " << _name << " type is null!";
			return false;
		}
		_type = node.Attribute("type");
		_dtype = ParseType(_type);
		if(_dtype == TYPE_NULL){
			ROUTN_LOG_ERROR(g_logger) << "index name = " << _name << " type = "
				<< _type << " invalid(pk,index,uniq)";
			return false;
		}

		if(!node.Attribute("cols")){
			ROUTN_LOG_ERROR(g_logger) << "index name = " << _name << " cols invalid";
			return false;
		}
		std::string tmp = node.Attribute("cols");
		_cols = Routn::split(tmp, ',');

		if(node.Attribute("desc")){
			_desc = node.Attribute("desc");
		}
		return true;
	}

	std::string Index::TypeToString(Type v){
#define XX(a, b) \
	if(v == a) { \
		return b; \
	} 
	XX(TYPE_PK, "pk");
	XX(TYPE_UNIQ, "uniq");
	XX(TYPE_INDEX, "index");
#undef XX
		return "";
	}

	Index::Type Index::ParseType(const std::string& v){
#define XX(a, b) \
	if(v == b){ \
		return a; \
	}
	XX(TYPE_PK, "pk");
	XX(TYPE_UNIQ, "uniq");
	XX(TYPE_INDEX, "index");
#undef XX
		return TYPE_NULL;
	}

}
}


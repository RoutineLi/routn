/*************************************************************************
	> File Name: util.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月23日 星期一 12时08分19秒
 ************************************************************************/

#include "OrmUtil.h"
#include "../Util.h"

namespace Routn{
namespace Orm{

	std::string GetAsVariable(const std::string& v){
		return ToLower(v);
	}

	std::string GetAsClassName(const std::string& v){
		auto vs = split(v, '_');
		std::stringstream ss;
		for(auto& i : vs){
			i[0] = toupper(i[0]);
			ss << i;
		}
		return ss.str();
	}

	std::string GetAsMemberName(const std::string& v){
		auto class_name = GetAsClassName(v);
		class_name[0] = tolower(class_name[0]);
		return "_" + class_name;
	}

	std::string GetAsGetFunName(const std::string& v){
		auto class_name = GetAsClassName(v);
		return "get" + class_name;
	}

	std::string GetAsSetFunName(const std::string& v){
		auto class_name = GetAsClassName(v);
		return "set" + class_name;		
	}

	std::string XmlToString(const TiXmlNode& node){
		return "";
	}

	std::string GetAsDefineMacro(const std::string& v){
		std::string tmp = replace(v, '.', '_');
		tmp = ToUpper(tmp);
		return "__" + tmp + "__";
	}

}
}



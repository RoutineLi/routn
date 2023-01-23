/*************************************************************************
	> File Name: OrmUtil.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月23日 星期一 12时08分15秒
 ************************************************************************/

#ifndef _ORM_UTIL_H
#define _ORM_UTIL_H

#include <tinyxml.h>
#include <string>

namespace Routn{
namespace Orm{

	std::string GetAsVariable(const std::string& v);
	std::string GetAsClassName(const std::string& v);
	std::string GetAsMemberName(const std::string& v);
	std::string GetAsGetFunName(const std::string& v);
	std::string GetAsSetFunName(const std::string& v);
	std::string XmlToString(const TiXmlNode& node);
	std::string GetAsDefineMacro(const std::string& v);

}
}

#endif

/*************************************************************************
	> File Name: Index.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月23日 星期一 12时36分46秒
 ************************************************************************/

#ifndef _INDEX_H
#define _INDEX_H

#include <memory>
#include <string>
#include <vector>
#include <tinyxml.h>

namespace Routn{
namespace Orm{
	
	class Index{
	public:
		enum Type{
			TYPE_NULL = 0,	//空字段
			TYPE_PK,		//主键
			TYPE_UNIQ,		//唯一
			TYPE_INDEX		//索引
		};
		using ptr = std::shared_ptr<Index>;
		const std::string& getName() const { return _name;}
		const std::string& getType() const { return _type;}
		const std::string& getDesc() const { return _desc;}
		const std::vector<std::string>& getCols() const { return _cols;}
		Type getDType() const { return _dtype;}

		bool init(const TiXmlElement& node);
		bool isPK() const { return _type == "pk";}

		static Type ParseType(const std::string& v);
		static std::string TypeToString(Type v);
	private:
		std::string _name;
		std::string _type;
		std::string _desc;
		std::vector<std::string> _cols;

		Type _dtype;
	};
}
}

#endif

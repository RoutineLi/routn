/*************************************************************************
	> File Name: Column.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月23日 星期一 12时37分03秒
 ************************************************************************/

#include "Column.h"
#include "../Log.h"
#include "OrmUtil.h"

namespace Routn{
namespace Orm{

	static Logger::ptr g_logger = ROUTN_LOG_NAME("orm");

	std::string Column::getDefaultValueString(){

	}

	std::string Column::getSQLite3Default(){

	}

	
	bool Column::init(const TiXmlElement& node){

	}


	std::string Column::getMemberDefine() const {

	}
	
	std::string Column::getGetFunDefine() const {

	}
	
	std::string Column::getSetFunDefine() const {

	}
	
	std::string Column::getSetFunImpl(const std::string& class_name, int idx) const {

	}


	Column::Type Column::ParseType(const std::string& v){

	}

	std::string Column::TypeToString(Type type){

	}

	std::string Column::getSQLite3TypeString(){

	}
	
	std::string Column::getMySQLTypeString(){

	}
	
	std::string Column::getBindString(){

	}
	
	std::string Column::getGetString(){

	}
	

}
}

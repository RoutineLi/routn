/*************************************************************************
	> File Name: Column.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月23日 星期一 12时37分03秒
 ************************************************************************/

#include "Column.h"
#include "../Log.h"
#include "OrmUtil.h"
#include <linux/io_uring.h>

namespace Routn{
namespace Orm{

	static Logger::ptr g_logger = ROUTN_LOG_NAME("orm");

	std::string Column::getDefaultValueString(){
		if(_default.empty()){
			return "";
		}
		if(_dtype <= TYPE_DOUBLE){
			return _default;
		}
		if(_dtype <= TYPE_BLOB){
			return "\"" + _default + "\"";
		}
		if(_default == "current_timestamp"){
			return "time(0)";
		}
		return "Routn::StringToTimer(\"" + _default + "\")";
	}

	std::string Column::getSQLite3Default(){
		if(_dtype <= TYPE_UINT64){
			if(_default.empty()){
				return "0";
			}
			return _default;
		}
		if(_dtype <= TYPE_BLOB){
			if(_default.empty()){
				return "''";
			}
			return "'" + _default + "'";
		}
		if(_default.empty()){
			return "'1980-01-01 00:00:00'";
		}
		return _default;
	}

	
	bool Column::init(const TiXmlElement& node){
		if(!node.Attribute("name")){
			ROUTN_LOG_ERROR(g_logger) << "column name not exists";
			return false;
		}
		_name = node.Attribute("name");
	
		if(!node.Attribute("type")){
			ROUTN_LOG_ERROR(g_logger) << "column name = " << _name
				<< " type is null";
			return false;
		}
		_type = node.Attribute("type");
		_dtype = ParseType(_type);
		if(_dtype == TYPE_NULL){
			ROUTN_LOG_ERROR(g_logger) << "column name = " << _name
				<< " type = " << _type
				<< " type is invalid";
			return false;
		}
		if(node.Attribute("desc")){
			_desc = node.Attribute("desc");
		}
		if(node.Attribute("default")){
			_default = node.Attribute("default");
		}
		if(node.Attribute("update")){
			_update = node.Attribute("update");
		}

		if(node.Attribute("length")){
			node.QueryIntAttribute("length", &_length);
		}else{
			_length = 0;
		}

		node.QueryBoolAttribute("auto_increment", &_autoIncrement);
		return true;
	}


	std::string Column::getMemberDefine() const {
		std::stringstream ss;
		ss << TypeToString(_dtype) << " " << GetAsMemberName(_name) << ";" << std::endl;
		return ss.str();
	}
	
	std::string Column::getGetFunDefine() const {
		std::stringstream ss;
		ss << "const " << TypeToString(_dtype) << "& " << GetAsGetFunName(_name)
			<< "() { return " << GetAsMemberName(_name) << "; }" << std::endl;
		return ss.str(); 
	}
	
	std::string Column::getSetFunDefine() const {
		std::stringstream ss;
		ss << "void " << GetAsSetFunName(_name) << "(const "
			<< TypeToString(_dtype) << "& v);" << std::endl;
		return ss.str();
	}
	
	std::string Column::getSetFunImpl(const std::string& class_name, int idx) const {
		std::stringstream ss;
		ss << "void " << GetAsClassName(class_name) << "::" << GetAsSetFunName(_name) << "(const "
		   << TypeToString(_dtype) <<"& v){" << std::endl;
		ss << "    " << GetAsMemberName(_name) << " = v;" << std::endl;
		ss << "}" << std::endl;
		return ss.str();
	}


	Column::Type Column::ParseType(const std::string& v){
#define XX(a, b, c) \
	if(#b == v){ \
		return a; \
	}else if(#c == v){ \
		return a; \
	}
		XX(TYPE_INT8, int8_t, int8);
		XX(TYPE_UINT8, uint8_t, uint8);
		XX(TYPE_INT16, int16_t, int16);
		XX(TYPE_UINT16, uint16_t, uint16);
		XX(TYPE_INT32, int32_t, int32);
		XX(TYPE_UINT32, uint32_t, uint32);
		XX(TYPE_FLOAT, float, float);
		XX(TYPE_INT64, int64_t, int64);
		XX(TYPE_UINT64, uint64_t, uint64);
		XX(TYPE_DOUBLE, double, double);
		XX(TYPE_STRING, string, std::string);
		XX(TYPE_TEXT, text, std::string);
		XX(TYPE_BLOB, blob, blob);
		XX(TYPE_TIMESTAMP, timestamp, datetime);
#undef XX
		return TYPE_NULL;	
	}

	std::string Column::TypeToString(Type type){
#define XX(a, b) \
	if(a == type) \
		return #b; 

		XX(TYPE_INT8, int8_t);
		XX(TYPE_UINT8, uint8_t);
		XX(TYPE_INT16, int16_t);
		XX(TYPE_UINT16, uint16_t);
		XX(TYPE_INT32, int32_t);
		XX(TYPE_UINT32, uint32_t);
		XX(TYPE_FLOAT, float);
		XX(TYPE_INT64, int64_t);
		XX(TYPE_UINT64, uint64_t);
		XX(TYPE_DOUBLE, double);
		XX(TYPE_STRING, std::string);
		XX(TYPE_TEXT, std::string);
		XX(TYPE_BLOB, std::string);
		XX(TYPE_TIMESTAMP, int64_t);		
#undef XX
		return "null";
	}

	std::string Column::getSQLite3TypeString(){
#define XX(a, b) \
	if(a == _dtype)\
		return #b;
	    XX(TYPE_INT8, INTEGER);
		XX(TYPE_UINT8, INTEGER);
		XX(TYPE_INT16, INTEGER);
		XX(TYPE_UINT16, INTEGER);
		XX(TYPE_INT32, INTEGER);
		XX(TYPE_UINT32, INTEGER);
		XX(TYPE_FLOAT, REAL);
		XX(TYPE_INT64, INTEGER);
		XX(TYPE_UINT64, INTEGER);
		XX(TYPE_DOUBLE, REAL);
		XX(TYPE_STRING, TEXT);
		XX(TYPE_TEXT, TEXT);
		XX(TYPE_BLOB, BLOB);
		XX(TYPE_TIMESTAMP, TIMESTAMP);	
#undef XX
		return "";
	}
	
	std::string Column::getMySQLTypeString(){
#define XX(a, b) \
	if(a == _dtype) \
		return #b; 
		XX(TYPE_INT8, tinyint);
		XX(TYPE_UINT8, tinyint unsigned);
		XX(TYPE_INT16, smallint);
		XX(TYPE_UINT16, smallint unsigned);
		XX(TYPE_INT32, int);
		XX(TYPE_UINT32, int unsigned);
		XX(TYPE_FLOAT, float);
		XX(TYPE_INT64, bigint);
		XX(TYPE_UINT64, bigint unsigned);
		XX(TYPE_DOUBLE, double);
		XX(TYPE_TEXT, text);
		XX(TYPE_BLOB, blob);
		XX(TYPE_TIMESTAMP, timestamp);	
#undef XX
		if(_dtype == TYPE_STRING){
			return "varchar(" + std::to_string(_length ? _length : 128) + ")";
		}
		return "";
	}
	
	std::string Column::getBindString(){
#define XX(a, b) \
	if(a == _dtype) \
		return "bind" #b;
	    XX(TYPE_INT8, Int8);
		XX(TYPE_UINT8, Uint8);
		XX(TYPE_INT16, Int16);
		XX(TYPE_UINT16, Uint16);
		XX(TYPE_INT32, Int32);
		XX(TYPE_UINT32, Uint32);
		XX(TYPE_FLOAT, Float);
		XX(TYPE_INT64, Int64);
		XX(TYPE_UINT64, Uint64);
		XX(TYPE_DOUBLE, Double);
		XX(TYPE_STRING, String);
		XX(TYPE_TEXT, String);
		XX(TYPE_BLOB, Blob);
		XX(TYPE_TIMESTAMP, Time);	
#undef XX
		return "";
	}
	
	std::string Column::getGetString(){
#define XX(a, b) \
	if(a == _dtype) \
		return "get" #b;
	    XX(TYPE_INT8, Int8);
		XX(TYPE_UINT8, Uint8);
		XX(TYPE_INT16, Int16);
		XX(TYPE_UINT16, Uint16);
		XX(TYPE_INT32, Int32);
		XX(TYPE_UINT32, Uint32);
		XX(TYPE_FLOAT, Float);
		XX(TYPE_INT64, Int64);
		XX(TYPE_UINT64, Uint64);
		XX(TYPE_DOUBLE, Double);
		XX(TYPE_STRING, String);
		XX(TYPE_TEXT, String);
		XX(TYPE_BLOB, Blob);
		XX(TYPE_TIMESTAMP, Time);
#undef XX
		return "";
	}
	

}
}

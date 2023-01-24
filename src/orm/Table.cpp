/*************************************************************************
	> File Name: Table.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月23日 星期一 12时35分15秒
 ************************************************************************/

#include "Table.h"
#include "../Log.h"
#include "OrmUtil.h"
#include <set>

namespace Routn{
namespace Orm{
	
	static Logger::ptr g_logger = ROUTN_LOG_NAME("orm");


	bool Table::init(const TiXmlElement& node){
		if(!node.Attribute("name")){
			ROUTN_LOG_ERROR(g_logger) << "table name is null";
			return false;
		}
		_name = node.Attribute("name");

		if(!node.Attribute("namespace")){
			ROUTN_LOG_ERROR(g_logger) << "table namespace is null";
			return false;
		}
		_namespace = node.Attribute("namespace");

		if(node.Attribute("desc")){
			_desc = node.Attribute("desc");
		}

		const TiXmlElement* cols = node.FirstChildElement("columns");
		if(!cols){
			ROUTN_LOG_ERROR(g_logger) << "table name = " << _name << " columns is null";
			return false;
		}

		const TiXmlElement* col = cols->FirstChildElement("column");
		if(!col){
			ROUTN_LOG_ERROR(g_logger) << "table name = " << _name << " column is null";
			return false;
		}

		std::set<std::string> col_names;
		int ind = 0;
		do{
			auto col_ptr = std::make_shared<Column>();
			if(!col_ptr->init(*col)){
				ROUTN_LOG_ERROR(g_logger) << "table name = " << _name << "init column is null";
				return false;
			}
			if(col_names.emplace(col_ptr->getName()).second == false){
				ROUTN_LOG_ERROR(g_logger) << "table name = " << _name << " column name = "
					<< col_ptr->getName() << " exists";
				return false;
			}
			col_ptr->_index = ind++;
			_cols.push_back(col_ptr);
			col = col->NextSiblingElement("column");
		}while(col);

		const TiXmlElement* idxs = node.FirstChildElement("indexs");
		if(!idxs){
			ROUTN_LOG_ERROR(g_logger) << "table name = " << _name << " indexs is null";
			return false;
		}

		const TiXmlElement* idx = idxs->FirstChildElement("index");
		if(!idx){
			ROUTN_LOG_ERROR(g_logger) << "table name = " << _name << " index is null";
			return false;
		}

		std::set<std::string> idx_names;
		bool has_pk = false;
		do{
			auto idx_ptr = std::make_shared<Index>();
			if(!idx_ptr->init(*idx)){
				ROUTN_LOG_ERROR(g_logger) << "table name = " << _name << " index init error";
				return false;
			}
			if(idx_names.emplace(idx_ptr->getName()).second == false){
				ROUTN_LOG_ERROR(g_logger) << "table name = " << _name << " index name = "
					<< idx_ptr->getName() << "exist";
				return false;
			}

			if(idx_ptr->isPK()){
				if(has_pk){
					ROUTN_LOG_ERROR(g_logger) << "table name = " << _name << " already has primary-key";
					return false;
				}
				has_pk = true;
			}

			auto& cnames = idx_ptr->getCols();
			for(auto& x : cnames){
				if(col_names.count(x) == 0){
					ROUTN_LOG_ERROR(g_logger) << "table name = " << _name << "idx = " << 
						idx_ptr->getName() << " col = " << x << " not exists";
					return false;
				}
			}

			_idxs.push_back(idx_ptr);
			idx = idx->NextSiblingElement("index");
		}while(idx);
		return true;
	}

	void Table::gen(const std::string& path){
		std::string p = path + "/" + Routn::replace(_namespace, ".", "/");
		Routn::FSUtil::Mkdir(p);
		gen_inc(p);			// auto-make xxx.h
		gen_src(p);			// auto-make xxx.cpp
	}

	std::string Table::getFilename() const {
		return ToLower(_name + _subfix);
	}


	void Table::gen_inc(const std::string& path){
		std::string filename = path + "/" + _name + _subfix + ".h";
		std::string class_name = _name + _subfix;
		std::string class_name_dao = _name + _subfix + "_dao";
		std::ofstream ofs(filename);
		ofs << "#ifndef" << GetAsDefineMacro(_namespace + class_name + ".h") << std::endl;
		ofs << "#define" << GetAsDefineMacro(_namespace + class_name + ".h") << std::endl;

		ofs << std::endl;
		ofs << "#include <vector>" << std::endl;
		ofs << "#include \"src/plugins/configor/json.hpp\"" << std::endl;

		std::set<std::string> incs = {"src/db/DateBase.h", "src/Util.h"};
		for(auto& i : incs){
			ofs << "#include \"" << i << "\"" << std::endl;
		} 
		ofs << std::endl;
		ofs << std::endl;

		std::vector<std::string> ns = split(_namespace, '.');
		for(auto x : ns){
			ofs << "namespace " << x << " {" << std::endl;
		}

		ofs << std::endl;
		ofs << "class " << GetAsClassName(class_name_dao) << ";" << std::endl;
		ofs << "class " << GetAsClassName(class_name) << " {" << std::endl;
		ofs << "friend class " << GetAsClassName(class_name_dao) << std::endl;
		ofs << "public: " << std::endl;
		ofs << "	using ptr = std::shared_ptr<" << GetAsClassName(class_name) << ">;" << std::endl;
		ofs << std::endl;
		ofs << "	" << GetAsClassName(class_name) << "();" << std::endl;
		ofs << std::endl;

		auto cols = _cols;
		std::sort(cols.begin(), cols.end(), [](const Column::ptr& a, const Column::ptr& b){
			if(a->getDType() != b->getDType()){
				return a->getDType() < b->getDType();
			}else{
				return a->getIndex() < b->getIndex();
			}
		});
	
		for(auto& i : _cols){
			ofs << "    " << i->getGetFunDefine();
			ofs << "    " << i->getSetFunDefine();
			ofs << std::endl;
		}
		ofs << "	" << genToStringInc() << std::endl;
		ofs << std::endl;

		ofs << "private: " << std::endl;
		for(auto& i : cols){
			ofs << "	" << i->getMemberDefine();
		}

		ofs << "};" << std::endl;
		ofs << std::endl;

		ofs << std::endl;
		gen_dao_inc(ofs);
		ofs << std::endl;

		for(auto x : ns){
			ofs << "} //namespace " << x << std::endl;
		}
		ofs << "#endif //" << GetAsDefineMacro(_namespace + class_name + ".h") << std::endl;
	}	

	void Table::gen_src(const std::string& path){
		std::string class_name = _name + _subfix;
		std::string filename = path + "/" + class_name + ".cpp";
		std::ofstream ofs(filename);

		ofs << "#include \"" << class_name + ".h\"" << std::endl;
		ofs << "#include \"src/Log.h\"" << std::endl;
		ofs << std::endl;
		ofs << "using namespace configor" << std::endl;
		ofs << std::endl;

		std::vector<std::string> ns = split(_namespace, ".");
		for(auto x : ns){
			ofs << "namespace " << x << " {" << std::endl;
		}

		ofs << std::endl;
		ofs << "static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME(\"orm\");" << std::endl;

		ofs << std::endl;
		ofs << GetAsClassName(class_name) << "::" << GetAsClassName(class_name) << "()" << std::endl;
		ofs << "    :";
		auto cols = _cols;
		std::sort(cols.begin(), cols.end(), [](const Column::ptr& a, const Column::ptr& b){
			if(a->getDType() != b->getDType()){
				return a->getDType() < b->getDType();
			}else{
				return a->getIndex() < b->getIndex();
			}
		});
		for(auto it = cols.begin(); it != cols.end(); ++it){
			if(it != cols.begin()){
				ofs << std::endl << "	,";
			}
			ofs << GetAsMemberName((*it)->getName()) << "("
			    << (*it)->getDefaultValueString() << ")";
		}
		ofs << " {" << std::endl;
		ofs << "}" << std::endl;
		ofs << std::endl;

		ofs << genToStringSrc(class_name) << std::endl;

		for(size_t i = 0; i < _cols.size(); ++i){
			ofs << _cols[i]->getSetFunImpl(class_name, i) << std::endl;
		}

		ofs << std::endl;
		gen_dao_src(ofs);
		ofs << std::endl;

		for(auto x : ns){
			ofs << "} //namespace " << x << std::endl;
		}
	}

	std::string Table::genToStringInc(){
		std::stringstream ss;
		ss << "std::string toJsonString() const ;";
		return ss.str();
	}

	std::string Table::genToStringSrc(const std::string& class_name){
		std::stringstream ss;
		ss << "std::string " << GetAsClassName(class_name) << "::toJsonString() const {";
		ss << "    json::value jvalue;" << std::endl;
		for(auto x : _cols){
			ss << "    jvalue[\""
			   << x->getName() << "\"] = ";
			if(x->getDType() == Column::TYPE_UINT64 || x->getDType() == Column::TYPE_INT64){
				ss << "std::to_string(" << GetAsMemberName(x->getName()) << ")"
					<< ";" << std::endl;
			}else if(x->getDType() == Column::TYPE_TIMESTAMP){
				ss << "Routn::TimerToString(" << GetAsMemberName(x->getName()) << ")"
					<< ";" << std::endl;
			}else{
				ss << GetAsMemberName(x->getName())
					<< ";" << std::endl;
			}
		}	
		ss << "    return Routn::JsonUtil::ToString(jvalue);" << std::endl;
		ss << "}" << std::endl;
		return ss.str();
	}

	std::string Table::genToInsertSQL(const std::string& class_name){
		std::stringstream ss;
		ss << "std::string " <<
			GetAsClassName(class_name)
		    << "::toInsertSQL() const {" << std::endl;
		ss << "    std::stringstream ss;" << std::endl;
		ss << "    ss << \"insert into " << _name
		   << "(";
		bool is_first = true;
		for(size_t i = 0; i < _cols.size(); ++i){
			if(_cols[i]->isAutoIncrement()){
				continue;
			}
			if(!is_first){
				ss << ",";
			}
			ss << _cols[i]->getName();
			is_first = false;
		}
		ss << ") values (\"" << std::endl;
		is_first = true;
		for(size_t i = 0; i < _cols.size(); ++i){
			if(_cols[i]->isAutoIncrement()){
				continue;
			}
			if(!is_first){
				ss << "    ss << \",\";" << std::endl;
			}
			if(_cols[i]->getDType() == Column::TYPE_STRING){
				ss << "    ss << \"'\" << Routn::replace("
				   << GetAsMemberName(_cols[i]->getName())
				   << ", \"'\", \"''\") << \"'\";" << std::endl;
			}else{
				ss << "    ss << " << GetAsMemberName(_cols[i]->getName())
					<< ";" << std::endl;
			}
			is_first = true;
		}
		ss << "    ss << \")\";" << std::endl;
		ss << "    return ss.str();" << std::endl;
		ss << "}" << std::endl;
		return ss.str();
	}

	std::string Table::genToUpdateSQL(const std::string& class_name){
		std::stringstream ss;
		ss << "std::string " << 
		GetAsClassName(class_name)
		<< "::toUpdateSQL() const {" << std::endl;
		ss << "    std::stringstream ss;" << std::endl;
		ss << "    bool is_first = true;" << std::endl;
		ss << "    ss << \"update " << _name
		<< " set \";" << std::endl;
		for(size_t i = 0; i < _cols.size(); ++i) {
			ss << "    if(_flags & " << (1ul << i) << "ul) {" << std::endl;
			ss << "        if(!is_first) {" << std::endl;
			ss << "            ss << \",\";" << std::endl;
			ss << "        }" << std::endl;
			ss << "        ss << \" " << _cols[i]->getName()
			<< " = ";
			if(_cols[i]->getDType() == Column::TYPE_STRING) {
				ss << "'\" << Routn::replace("
				<< GetAsMemberName(_cols[i]->getName())
				<< ", \"'\", \"''\") << \"'\";" << std::endl;
			} else {
				ss << "\" << " << GetAsMemberName(_cols[i]->getName())
				<< ";" << std::endl;
			}
			ss << "        is_first = false;" << std::endl;
			ss << "    }" << std::endl;
		}
		ss << genWhere();
		ss << "    return ss.str();" << std::endl;
		ss << "}" << std::endl;
		return ss.str();
	}

	std::string Table::genToDeleteSQL(const std::string& class_name){
		std::stringstream ss;
		ss << "std::string " << 
		GetAsClassName(class_name)
		<< "::toDeleteSQL() const {" << std::endl;
		ss << "    std::stringstream ss;" << std::endl;
		ss << "    ss << \"delete from " << _name << "\";" << std::endl;
		ss << genWhere();
		ss << "    return ss.str();" << std::endl;
		ss << "}" << std::endl;
		return ss.str();
	}


	std::vector<Column::ptr> Table::getPKs() const {
		std::vector<Column::ptr> cols;
		for(auto& i : _idxs){
			if(i->isPK()){
				for(auto& n : i->getCols()){
					cols.push_back(getCol(n));
				}
			}
		}
		return cols;
	}

	Column::ptr Table::getCol(const std::string& name) const {
		for(auto& i : _cols){
			if(i->getName() == name){
				return i;
			}
		}
		return nullptr;
	}


	std::string Table::genWhere() const {
		std::stringstream ss;
		ss << "    ss << \" where ";
		auto pks = getPKs();

		for(size_t i = 0; i < pks.size(); ++i) {
			if(i) {
				ss << "    ss << \" and ";
			}
			ss << pks[i]->getName() << " = ";
			if(pks[i]->getDType() == Column::TYPE_STRING) {
				ss << "'\" << Routn::replace("
				<< GetAsMemberName(_cols[i]->getName())
				<< ", \"'\", \"''\") << \"'\";" << std::endl;
			} else {
				ss << "\" << " << GetAsMemberName(_cols[i]->getName()) << ";" << std::endl;
			}
		}
		return ss.str();
	}

	void Table::gen_dao_inc(std::ofstream& ofs){
		std::string class_name = _name + _subfix;
		std::string class_name_dao = class_name + "_dao";
		ofs << "class " << GetAsClassName(class_name_dao) << " {" << std::endl;
		ofs << "public: " << std::endl;
		ofs << "    using ptr = std::shared_ptr<" << GetAsClassName(class_name_dao) << "> ;" << std::endl;
		ofs << "    static int Update(" << GetAsClassName(class_name) 
			<< "::ptr info, " << _updateclass << "::ptr conn);" << std::endl;
		ofs << "    static int Insert(" << GetAsClassName(class_name)
			<< "::ptr info, " << _updateclass << "::ptr conn);" << std::endl;
		ofs << "    static int Delete(" << GetAsClassName(class_name)
		    << "::ptr info, " << _updateclass << "::ptr conn);" << std::endl;
		auto vs = getPKs();
		ofs << "    static int Delete(";
		for(auto& i : vs){
			ofs << "const " << i->getDTypeString() << "& "
				<< GetAsVariable(i->getName()) << ", ";
		}
		ofs << _updateclass << "::ptr conn);" << std::endl;

		for(auto &i : _idxs){
			if(i->getDType() == Index::TYPE_UNIQ
				|| i->getDType() == Index::TYPE_PK
				|| i->getDType() == Index::TYPE_INDEX){
				ofs << "	static int Delete";
				std::string temp = "by";
				for(auto& c : i->getCols()){
					temp += "_" + c;
				}
				ofs << GetAsClassName(temp) << "(";
				for(auto& c : i->getCols()){
					auto d = getCol(c);
					ofs << " const " << d->getDTypeString() << "& "
						<< GetAsVariable(d->getName()) << ", ";
				}
				ofs << _updateclass << "::ptr conn);" << std::endl;
			}
		}

		ofs << "	static int QueryAll(std::vector<"
			<< GetAsClassName(class_name) << "::ptr>& results, " << _queryclass
			<< "::ptr conn);" << std::endl;
		ofs << "	static " << GetAsClassName(class_name) << "::ptr Query(";
		for(auto& i : vs){
			ofs << " const " << i->getDTypeString() << "& "
				<< GetAsVariable(i->getName()) << ", ";
		}
		ofs << _queryclass << "::ptr conn);" << std::endl;

		for(auto& i : _idxs){
			if(i->getDType() == Index::TYPE_UNIQ){
				ofs << "	static " << GetAsClassName(class_name) << "::ptr Query";
				std::string temp = "by";
				for(auto &c : i->getCols()){
					temp += "_" + c;
				}
				ofs << GetAsClassName(temp) << "(";
				for(auto& c : i->getCols()){
					auto d = getCol(c);
					ofs << " const " << d->getDTypeString() << "& "
						<< GetAsVariable(d->getName()) << ", ";
				}
				ofs << _queryclass << "::ptr conn);" << std::endl;
			}else if(i->getDType() == Index::TYPE_INDEX){
				ofs << "    static int Query";
				std::string tmp = "by";
				for(auto& c : i->getCols()) {
					tmp += "_" + c;
				}
				ofs << GetAsClassName(tmp) << "(";
				ofs << "std::vector<" << GetAsClassName(class_name) << "::ptr>& results, ";
				for(auto& c : i->getCols()) {
					auto d = getCol(c);
					ofs << " const " << d->getDTypeString() << "& "
						<< GetAsVariable(d->getName()) << ", ";
           		}
				ofs << _queryclass << "::ptr conn);" << std::endl;
			}
		}

		ofs << "	static int CreateTableSQLite3(" << _dbclass << "::ptr info);" << std::endl;
		ofs << "    static int CreateTableMySQL(" << _dbclass << "::ptr info);" << std::endl;
		ofs << "};" << std::endl;
	}

	template<class V, class T>
	bool is_exists(const V& v, const T& t){
		for(auto &i : v){
			if(i == t){
				return true;
			}
		}
		return false;
	}

	void Table::gen_dao_src(std::ofstream& ofs){
		std::string class_name = _name + _subfix;
		std::string class_name_dao = class_name + "_dao";
		ofs << "int " << GetAsClassName(class_name_dao)
			<< "::Update(" << GetAsClassName(class_name)
			<< "::ptr info, " << _updateclass << "::ptr conn){" << std::endl;
		ofs << "	std::string sql = \"update " << _name << " set";
		auto pks = getPKs();
		bool is_first = true;
		for(auto& i : _cols){
			if(is_exists(pks, i)){
				continue;
			}
			if(!is_first){
				ofs << ", ";
			}
			ofs << " " << i->getName() << " = ?";
			is_first = false;
		}

		ofs << " where";
		is_first = true;
		for(auto& i : pks){
			if(!is_first){
				ofs << " and";
			}
			ofs << " " << i->getName() << " = ?";
		}
		ofs << "\";" << std::endl;
#define CHECK_STMT(v) \
	ofs << "	auto stmt = conn->prepare(sql);" << std::endl; \
	ofs << "	if(!stmt){" << std::endl; \
	ofs << "		ROUTN_LOG_ERROR(g_logger) << \"stmt =\" << sql" << std::endl << \
		"				<< \" errno = \"" \
		" << conn->getErrno() << \" reason:\" << conn->getErrStr();" << std::endl \
		<< "		return " v ";" << std::endl;\
	ofs << "	}" << std::endl;


		CHECK_STMT("conn->getErrno()");
		is_first = true;
		int idx = 1;
		for(auto& i : _cols){
			if(is_exists(pks, i)){
				continue;
			}
			ofs << "	stmt->" << i->getBindString() << "(" << idx << ", ";
			ofs << "info->" << GetAsMemberName(i->getName());
			ofs << ");" << std::endl;
			++idx;
		}
		for(auto& i : pks){
			ofs << "	stmt->" << i->getBindString() << "(" << idx << ", ";
			ofs << "info->" << GetAsMemberName(i->getName()) << ");" << std::endl;
			++idx;
		}
		ofs << "	return stmt->execute();" << std::endl;
		ofs << "}" << std::endl << std::endl;
		ofs << "int " << GetAsClassName(class_name_dao)
			<< "::Insert(" << GetAsClassName(class_name)
			<< "::ptr info, " << _updateclass << "::ptr conn){" << std::endl;
		ofs << "	std::string sql = \"insert into " << _name << " (";
		is_first = true;
		Column::ptr auto_inc;

		for(auto& i : _cols){
			if(i->isAutoIncrement()){
				auto_inc = i;
				continue;
			}
			if(!is_first){
				ofs << ", ";
			}
			ofs << i->getName();
			is_first = false;
		} 

		ofs << ") values (";
		is_first = true;
		for(auto& i : _cols){
			if(i->isAutoIncrement()){
				continue;
			}
			if(!is_first){
				ofs << ", ";
			}
			ofs << "?";
			is_first = false;
		}
		ofs << ")\";" << std::endl;

		CHECK_STMT("conn->getErrno()");

		idx = 1;
		for(auto &i : _cols){
			if(i->isAutoIncrement()){
				continue;
			}
			ofs << "	stmt->" << i->getBindString() << "(" << idx << ", ";
			ofs << "info->" << GetAsMemberName(i->getName());
			ofs << ");" << std::endl;
			++idx;
		}
		ofs << "	int rt = stmt->execute();" << std::endl;
		if(auto_inc){
			ofs << "	if(rt == 0){" << std::endl;
			ofs << "		info->" << GetAsMemberName(auto_inc->getName())
				<< " = conn->getLastInsertId();" << std::endl
				<< "	}" << std::endl;
		}
		ofs << "	return rt;" << std::endl;
		ofs << "}" << std::endl << std::endl;

		ofs << "int " << GetAsClassName(class_name_dao)
			<< "::InsertOrUpdate(" << GetAsClassName(class_name)
			<< "::ptr info, " << _updateclass << "::ptr conn){" << std::endl;
		for(auto& i : _cols){
			if(i->isAutoIncrement()){
				auto_inc = i;
				break;
			}
		}
		if(auto_inc){
			ofs << "	if(info->" << GetAsMemberName(auto_inc->getName()) << " == 0)" << std::endl;
			ofs << "		return Insert(info, conn);" << std::endl;
			ofs << "	}" << std::endl;
		}
		ofs << "	std::string sql = \"replace into " << _name << " (";
		is_first = true;
		for(auto& i : _cols){
			if(!is_first){
				ofs << ", ";
			}
			ofs << i->getName();
			is_first = false;
		}

		ofs << ") values (";
		is_first = true;
		for(auto& i : _cols){
			(void)i;
			if(!is_first){
				ofs << ", ";
			}
			ofs << "?";
			is_first = false;
		}
		ofs << ")\";" << std::endl;

		CHECK_STMT("conn->getErrno()");
		idx = 1;
		for(auto& i : _cols){
			ofs << " stmt->" << i->getBindString() << "(" << idx << ", ";
			ofs << "info->" << GetAsMemberName(i->getName()) << ");" << std::endl;
			++idx;
		}
		ofs << "	retrn stmt->execute();" << std::endl;
		ofs << "}" << std::endl << std::endl;

		ofs << "int " << GetAsClassName(class_name_dao)
			<< "::Delete(" << GetAsClassName(class_name)
			<< "::ptr info, " << _updateclass << "::ptr conn) {" << std::endl;
		
		ofs << "	std::string sql = \"delete from " << _name << " where";
		is_first = true;
		for(auto& i : pks){
			if(!is_first){
				ofs << " and";
			}
			ofs << " " << i->getName() << " = ?";
			is_first = false;
		}
		ofs << "\";" << std::endl;
		CHECK_STMT("conn->getErrno()");
		idx = 1;
		for(auto& i : pks){
			ofs << "	stmt->" << i->getBindString() << "(" << idx << ", ";
			ofs << "info->" << GetAsMemberName(i->getName()) << ");" << std::endl;
			++idx;
		}
		ofs << "	return stmt->execute();" << std::endl;
		ofs << "}" << std::endl << std::endl;

		for(auto& i : _idxs){
			if(i->getDType() == Index::TYPE_UNIQ
				|| i->getDType() == Index::TYPE_PK
				|| i->getDType() == Index::TYPE_INDEX){
				ofs << "int " << GetAsClassName(class_name_dao) << "::Delete";
				ofs << "int " << GetAsClassName(class_name_dao) << "::Delete";
				std::string tmp = "by";
				for(auto& c : i->getCols()) {
					tmp += "_" + c;
				}
				ofs << GetAsClassName(tmp) << "(";
				for(auto& c : i->getCols()) {
					auto d = getCol(c);
					ofs << " const " << d->getDTypeString() << "& "
						<< GetAsVariable(d->getName()) << ", ";
				}
				ofs << _updateclass << "::ptr conn) {" << std::endl;
				ofs << "    std::string sql = \"delete from " << _name << " where";
				is_first = true;
				for(auto& x : i->getCols()) {
					if(!is_first) {
						ofs << " and";
					}
					ofs << " " << x << " = ?";
					is_first = false;
				}
				ofs << "\";" << std::endl;
				CHECK_STMT("conn->getErrno()");
				idx = 1;
				for(auto& x : i->getCols()) {
					ofs << "    stmt->" << getCol(x)->getBindString() << "(" << idx << ", ";
					ofs << GetAsVariable(x) << ");" << std::endl;
				}
				ofs << "    return stmt->execute();" << std::endl;
				ofs << "}" << std::endl << std::endl;				
			}
		}

		ofs << "int " << GetAsClassName(class_name_dao) << "::QueryAll(std::vector<"
			<< GetAsClassName(class_name) << "::ptr>& results, "
			<< _queryclass << "::ptr conn){" << std::endl;
		ofs << "	std::string sql = \"select ";
		is_first = true;
		for(auto& i : _cols){
			if(!is_first){
				ofs << ", ";
			}
			ofs << i->getName();
			is_first = false;
		}
		ofs << " from " << _name << "\";" << std::endl;
		CHECK_STMT("conn->getErrno()");
		ofs << "    auto rt = stmt->query();" << std::endl;
		ofs << "    if(!rt) {" << std::endl;
		ofs << "        return stmt->getErrno();" << std::endl;
		ofs << "    }" << std::endl;
		ofs << "    while (rt->next()) {" << std::endl;
		ofs << "        " << GetAsClassName(class_name) << "::ptr v(new "
			<< GetAsClassName(class_name) << ");" << std::endl;	
#define PARSE_OBJ(prefix) \
	for(size_t i = 0; i < _cols.size(); ++i) {\
		ofs << prefix "v->" << GetAsMemberName(_cols[i]->getName()) << " = "; \
		ofs << "rt->" << _cols[i]->getGetString() << "(" << (i) << ");" << std::endl;\
	}	
		PARSE_OBJ("        ");
		ofs << "        results.push_back(v);" << std::endl;
		ofs << "    }" << std::endl;
		ofs << "    return 0;" << std::endl;
		ofs << "}" << std::endl << std::endl;

		ofs << GetAsClassName(class_name) << "::ptr "
			<< GetAsClassName(class_name_dao) << "::Query(";
		for(auto& i : pks){
			ofs << " const " << i->getDTypeString() << "& "
				<< GetAsVariable(i->getName()) << ", ";
		} 	
		ofs << _queryclass << "::ptr conn) {" << std::endl;

		ofs << "    std::string sql = \"select ";
		is_first = true;
		for(auto& i : _cols) {
			if(!is_first) {
				ofs << ", ";
			}
			ofs << i->getName();
			is_first = false;
		}
		ofs << " from " << _name << " where";
		is_first = true;
		for(auto& i : pks) {
			if(!is_first) {
				ofs << " and";
			}
			ofs << " " << i->getName() << " = ?";
			is_first = false;
		}
		ofs << "\";" << std::endl;

		CHECK_STMT("nullptr");
		idx = 1;
		for(auto& i : pks) {
			ofs << "    stmt->" << i->getBindString() << "(" << idx << ", ";
			ofs << GetAsVariable(i->getName()) << ");" << std::endl;
			++idx;
		}
		ofs << "    auto rt = stmt->query();" << std::endl;
		ofs << "    if(!rt) {" << std::endl;
		ofs << "        return nullptr;" << std::endl;
		ofs << "    }" << std::endl;
		ofs << "    if(!rt->next()) {" << std::endl;
		ofs << "        return nullptr;" << std::endl;
		ofs << "    }" << std::endl;
		ofs << "    " << GetAsClassName(class_name) << "::ptr v(new "
			<< GetAsClassName(class_name) << ");" << std::endl;
		PARSE_OBJ("    ");
		ofs << "    return v;" << std::endl;
		ofs << "}" << std::endl << std::endl;	

		for(auto& i : _idxs) {
			if(i->getDType() == Index::TYPE_UNIQ) {
				ofs << "" << GetAsClassName(class_name) << "::ptr "
					<< GetAsClassName(class_name_dao) << "::Query";
				std::string tmp = "by";
				for(auto& c : i->getCols()) {
					tmp += "_" + c;
				}
				ofs << GetAsClassName(tmp) << "(";
				for(auto& c : i->getCols()) {
					auto d = getCol(c);
					ofs << " const " << d->getDTypeString() << "& "
						<< GetAsVariable(d->getName()) << ", ";
				}
				ofs << _queryclass << "::ptr conn) {" << std::endl;

				ofs << "    std::string sql = \"select ";
				is_first = true;
				for(auto& i : _cols) {
					if(!is_first) {
						ofs << ", ";
					}
					ofs << i->getName();
					is_first = false;
				}
				ofs << " from " << _name << " where";
				is_first = true;
				for(auto& x : i->getCols()) {
					if(!is_first) {
						ofs << " and";
					}
					ofs << " " << x << " = ?";
					is_first = false;
				}
				ofs << "\";" << std::endl;
				CHECK_STMT("nullptr");

				idx = 1;
				for(auto& x : i->getCols()) {
					ofs << "    stmt->" << getCol(x)->getBindString() << "(" << idx << ", ";
					ofs << GetAsVariable(x) << ");" << std::endl;
					++idx;
				}
				ofs << "    auto rt = stmt->query();" << std::endl;
				ofs << "    if(!rt) {" << std::endl;
				ofs << "        return nullptr;" << std::endl;
				ofs << "    }" << std::endl;
				ofs << "    if(!rt->next()) {" << std::endl;
				ofs << "        return nullptr;" << std::endl;
				ofs << "    }" << std::endl;
				ofs << "    " << GetAsClassName(class_name) << "::ptr v(new "
					<< GetAsClassName(class_name) << ");" << std::endl;
				PARSE_OBJ("    ");
				ofs << "    return v;" << std::endl;
				ofs << "}" << std::endl << std::endl;
			} else if(i->getDType() == Index::TYPE_INDEX) {
				ofs << "int " << GetAsClassName(class_name_dao) << "::Query";
				std::string tmp = "by";
				for(auto& c : i->getCols()) {
					tmp += "_" + c;
				}
				ofs << GetAsClassName(tmp) << "(";
				ofs << "std::vector<" << GetAsClassName(class_name) << "::ptr>& results, ";
				for(auto& c : i->getCols()) {
					auto d = getCol(c);
					ofs << " const " << d->getDTypeString() << "& "
						<< GetAsVariable(d->getName()) << ", ";
				}
				ofs << _queryclass << "::ptr conn) {" << std::endl;

				ofs << "    std::string sql = \"select ";
				is_first = true;
				for(auto& i : _cols) {
					if(!is_first) {
						ofs << ", ";
					}
					ofs << i->getName();
					is_first = false;
				}
				ofs << " from " << _name << " where";
				is_first = true;
				for(auto& x : i->getCols()) {
					if(!is_first) {
						ofs << " and";
					}
					ofs << " " << x << " = ?";
					is_first = false;
				}
				ofs << "\";" << std::endl;
				CHECK_STMT("conn->getErrno()");

				idx = 1;
				for(auto& x : i->getCols()) {
					ofs << "    stmt->" << getCol(x)->getBindString() << "(" << idx << ", ";
					ofs << GetAsVariable(x) << ");" << std::endl;
					++idx;
				}
				ofs << "    auto rt = stmt->query();" << std::endl;
				ofs << "    if(!rt) {" << std::endl;
				ofs << "        return 0;" << std::endl;
				ofs << "    }" << std::endl;
				ofs << "    while (rt->next()) {" << std::endl;
				ofs << "        " << GetAsClassName(class_name) << "::ptr v(new "
					<< GetAsClassName(class_name) << ");" << std::endl;
				PARSE_OBJ("        ");
				ofs << "        results.push_back(v);" << std::endl;
				ofs << "    };" << std::endl;
				ofs << "    return 0;" << std::endl;
				ofs << "}" << std::endl << std::endl;
			}
		}

		ofs << "int " << GetAsClassName(class_name_dao) << "::CreateTableSQLite3(" << _dbclass << "::ptr conn) {" << std::endl;
		ofs << "    return conn->execute(\"CREATE TABLE " << _name << "(\"" << std::endl;
		is_first = true;
		bool has_auto_increment = false;
		for(auto& i : _cols) {
			if(!is_first) {
				ofs << ",\"" << std::endl;
			}
			ofs << "            \"" << i->getName() << " " << i->getSQLite3TypeString();
			if(i->isAutoIncrement()) {
				ofs << " PRIMARY KEY AUTOINCREMENT";
				has_auto_increment = true;
			} else {
				ofs << " NOT NULL DEFAULT " << i->getSQLite3Default();
			}
			is_first = false;
		}
		if(!has_auto_increment) {
			ofs << ", PRIMARY KEY(";
			is_first = true;
			for(auto& i : pks) {
				if(!is_first) {
					ofs << ", ";
				}
				ofs << i->getName();
			}
			ofs << ")";
		}
		ofs << ");\"" << std::endl;
		for(auto& i : _idxs) {
			if(i->getDType() == Index::TYPE_PK) {
				continue;
			}
			ofs << "            \"CREATE";
			if(i->getDType() == Index::TYPE_UNIQ) {
				ofs << " UNIQUE";
			}
			ofs << " INDEX " << _name;
			for(auto& x : i->getCols()) {
				ofs << "_" << x;
			}
			ofs << " ON " << _name
				<< "(";
			is_first = true;
			for(auto& x : i->getCols()) {
				if(!is_first) {
					ofs << ",";
				}
				ofs << x;
				is_first = false;
			}
			ofs << ");\"" << std::endl;
		}
		ofs << "            );" << std::endl;
		ofs << "}" << std::endl << std::endl;

		ofs << "int " << GetAsClassName(class_name_dao) << "::CreateTableMySQL(" << _dbclass << "::ptr conn) {" << std::endl;
		ofs << "    return conn->execute(\"CREATE TABLE " << _name << "(\"" << std::endl;
		is_first = true;
		for(auto& i : _cols) {
			if(!is_first) {
				ofs << ",\"" << std::endl;
			}
			ofs << "            \"`" << i->getName() << "` " << i->getMySQLTypeString();
			if(i->isAutoIncrement()) {
				ofs << " AUTO_INCREMENT";
				has_auto_increment = true;
			} else {
				ofs << " NOT NULL DEFAULT " << i->getSQLite3Default();
			}

			if(!i->getUpdate().empty()) {
				ofs << " ON UPDATE " << i->getUpdate() << " ";
			}
			if(!i->getDesc().empty()) {
				ofs << " COMMENT '" << i->getDesc() << "'";
			}
			is_first = false;
		}
		ofs << ",\"" << std::endl << "            \"PRIMARY KEY(";
		is_first = true;
		for(auto& i : pks) {
			if(!is_first) {
				ofs << ", ";
			}
			ofs << "`" << i->getName() << "`";
		}
		ofs << ")";
		for(auto& i : _idxs) {
			if(i->getDType() == Index::TYPE_PK) {
				continue;
			}
			ofs << ",\"" << std::endl;
			if(i->getDType() == Index::TYPE_UNIQ) {
				ofs << "            \"UNIQUE ";
			} else {
				ofs << "            \"";
			}
			ofs << "KEY `" << _name;
			for(auto& x : i->getCols()) {
				ofs << "_" << x;
			}
			ofs << "` (";
			is_first = true;
			for(auto& x : i->getCols()) {
				if(!is_first) {
					ofs << ",";
				}
				ofs << "`" << x << "`";
				is_first = false;
			}
			ofs << ")";
		}
		ofs << ")";
		if(!_desc.empty()) {
			ofs << " COMMENT='" << _desc << "'";
		}
		ofs << "\");" << std::endl;
		ofs << "}";		

 	}

}
}


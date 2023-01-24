/*************************************************************************
	> File Name: Table.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年01月23日 星期一 12时35分11秒
 ************************************************************************/

#ifndef _TABLE_H
#define _TABLE_H

#include "Column.h"
#include "Index.h"
#include <fstream>


namespace Routn{
namespace Orm{
	
	class Table{
	public:
		using ptr = std::shared_ptr<Table>;
		const std::string& getName() const { return _name;}
		const std::string& getNameSpace() const { return _namespace;}
		const std::string& getDesc() const { return _desc;}

		const std::vector<Column::ptr>& getCols() const { return _cols;}
		const std::vector<Index::ptr>& getIdxs() const { return _idxs;}

		bool init(const TiXmlElement& node);

		void gen(const std::string& path);

		std::string getFilename() const ;
	private:
		void gen_inc(const std::string& path);
		void gen_src(const std::string& path);
		std::string genToStringInc();
		std::string genToStringSrc(const std::string& class_name);
		std::string genToInsertSQL(const std::string& class_name);
		std::string genToUpdateSQL(const std::string& class_name);
		std::string genToDeleteSQL(const std::string& class_name);

		std::vector<Column::ptr> getPKs() const ;
		Column::ptr getCol(const std::string& name) const ;

		std::string genWhere() const ;

		void gen_dao_inc(std::ofstream& ofs);
		void gen_dao_src(std::ofstream& ofs);

		enum DBType{
			TYPE_SQLITE3 = 1,
			TYPE_MYSQL = 2
		};
	private:
		std::string _name;
		std::string	_namespace;
		std::string _desc;
		std::string _subfix = "_info";
		DBType _type = TYPE_SQLITE3;
		std::string _dbclass = "Routn::IDB";
		std::string _queryclass = "Routn::IDB";
		std::string _updateclass = "Routn::IDB";
		std::vector<Column::ptr> _cols;
		std::vector<Index::ptr> _idxs;
	};

}
}

#endif

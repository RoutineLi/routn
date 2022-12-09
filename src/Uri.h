/*************************************************************************
	> File Name: Uri.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月24日 星期四 01时15分51秒
 ************************************************************************/

#ifndef _URI_H
#define _URI_H

#include <memory>
#include <string>
#include <cstdint>
#include "Address.h"

namespace Routn{

	/** Uri格式语法如下:
	*	https://user@example.com:8083/over/there?name=routnli#nosee 
	*	\___/ \_____________________/\_________/\___________/ \___/
	*	  |    			|				  |			  |		    |
	*	scheme	userinfo+hosts+port		path		query	  fragment
	**/

	class Uri {		//Uniform-Resources-Identifier-Class
	public:
		using ptr = std::shared_ptr<Uri>;

		static Uri::ptr Create(const std::string& uristr);
		Uri();

		const std::string& getScheme() const { return _scheme; }
		const std::string& getUserinfo() const{ return _userinfo;}
		const std::string& getHost() const { return _host;}
		const std::string& getQuery() const { return _query;}
		const std::string& getFragment() { return _fragment;}
		const std::string& getPath() const;
		const int32_t getPort() const;

		void setScheme(const std::string& v) { _scheme = v;}
		void setUserinfo(const std::string& v) { _userinfo = v;}
		void setHost(const std::string& v) { _host = v;}
		void setPath(const std::string& v) { _path = v;}
		void setQuery(const std::string& v) { _query = v;}
		void setFragment(const std::string& v) { _fragment = v;}
		void setPort(const int32_t& v) { _port = v;}

		std::ostream& dump(std::ostream& os) const;
		std::string toString() const;

		Address::ptr createAddress() const;	//通过uri获取地址
	private:
		bool isDefaultPort() const;		//判断当前端口是否为默认端口
	private:
		std::string _scheme;
		std::string _userinfo;
		std::string _host;
		std::string _path;
		std::string _query;
		std::string _fragment;
		int32_t _port;
	};
}

#endif

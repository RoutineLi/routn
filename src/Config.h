/*************************************************************************
	> File Name: config.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月26日 星期三 15时35分15秒
 ************************************************************************/

#ifndef _CONFIG_H
#define _CONFIG_H

#include "Log.h"
#include "Thread.h"

#include <memory>
#include <functional>
#include <map>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <algorithm>
#include <yaml-cpp/yaml.h>
#include <boost/lexical_cast.hpp>

namespace Routn
{	

	static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

	class ConfigVarBase
	{
	public:
		using ptr = std::shared_ptr<ConfigVarBase>;
		ConfigVarBase(const std::string &name, const std::string &des = "")
			: _name(name), _description(des)
		{
			//统一转换成小写
			std::transform(_name.begin(), _name.end(), _name.begin(), ::tolower);
		}
		virtual ~ConfigVarBase() {}

		const std::string &getName() const { return _name; }
		const std::string &getDescription() const { return _description; }

		virtual std::string toString() = 0;
		virtual bool fromString(const std::string &val) = 0;
		virtual std::string getTypeName() const = 0;

	protected:
		std::string _name;
		std::string _description;
	};

	/**
	 * 类型转换封装基于Boost::lexical_cast
	**/
	template <class F, class T>
	class LexicalCast
	{
	public:
		T operator()(const F &v)
		{
			return boost::lexical_cast<T>(v);
		}
	};

	/**
	 * 模板类特化 STL
	**/
	//std::string to std::vector
	template <class T>
	class LexicalCast<std::string, std::vector<T>>
	{
	public:
		std::vector<T> operator()(const std::string &v)
		{
			YAML::Node node = YAML::Load(v);
			typename std::vector<T> vec;
			std::stringstream ss;
			for (size_t i = 0; i < node.size(); i++)
			{
				ss.str("");
				ss << node[i];
				vec.push_back(LexicalCast<std::string, T>()(ss.str()));
			}
			return vec;
		}
	};
	//std::vector to std::string
	template <class T>
	class LexicalCast<std::vector<T>, std::string>
	{
	public:
		std::string operator()(const std::vector<T> &v)
		{
			YAML::Node node;
			for (auto &i : v)
			{
				node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	//std::string to std::list
	template <class T>
	class LexicalCast<std::string, std::list<T>>
	{
	public:
		std::list<T> operator()(const std::string &v)
		{
			YAML::Node node = YAML::Load(v);
			typename std::list<T> lists;
			std::stringstream ss;
			for (size_t i = 0; i < node.size(); i++)
			{
				ss.str("");
				ss << node[i];
				lists.push_back(LexicalCast<std::string, T>()(ss.str()));
			}
			return lists;
		}
	};
	//std::list to std::string
	template <class T>
	class LexicalCast<std::list<T>, std::string>
	{
	public:
		std::string operator()(const std::list<T> &v)
		{
			YAML::Node node;
			for (auto &i : v)
			{
				node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	//std::string to std::set
	template <class T>
	class LexicalCast<std::string, std::set<T>>
	{
	public:
		std::set<T> operator()(const std::string &v)
		{
			YAML::Node node = YAML::Load(v);
			typename std::set<T> sets;
			std::stringstream ss;
			for (size_t i = 0; i < node.size(); i++)
			{
				ss.str("");
				ss << node[i];
				sets.emplace(LexicalCast<std::string, T>()(ss.str()));
			}
			return sets;
		}
	};
	//std::set to std::string
	template <class T>
	class LexicalCast<std::set<T>, std::string>
	{
	public:
		std::string operator()(const std::set<T> &v)
		{
			YAML::Node node;
			for (auto &i : v)
			{
				node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	//std::string to std::unordered_set
	template <class T>
	class LexicalCast<std::string, std::unordered_set<T>>
	{
	public:
		std::unordered_set<T> operator()(const std::string &v)
		{
			YAML::Node node = YAML::Load(v);
			typename std::unordered_set<T> sets;
			std::stringstream ss;
			for (size_t i = 0; i < node.size(); i++)
			{
				ss.str("");
				ss << node[i];
				sets.emplace(LexicalCast<std::string, T>()(ss.str()));
			}
			return sets;
		}
	};
	//std::unordered_set to std::string
	template <class T>
	class LexicalCast<std::unordered_set<T>, std::string>
	{
	public:
		std::string operator()(const std::unordered_set<T> &v)
		{
			YAML::Node node;
			for (auto &i : v)
			{
				node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	//std::string to std::map
	template <class T>
	class LexicalCast<std::string, std::map<std::string, T>>
	{
	public:
		std::map<std::string, T> operator()(const std::string &v)
		{
			YAML::Node node = YAML::Load(v);
			typename std::map<std::string, T> maps;
			std::stringstream ss;
			for (auto it = node.begin(); it != node.end(); ++it)
			{
				ss.str("");
				ss << it->second;
				maps.emplace(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
			}
			return maps;
		}
	};
	//std::map to std::string
	template <class T>
	class LexicalCast<std::map<std::string, T>, std::string>
	{
	public:
		std::string operator()(const std::map<std::string, T> &v)
		{
			YAML::Node node;
			for (auto &i : v)
			{
				node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	//std::string to std::unordered_map
	template <class T>
	class LexicalCast<std::string, std::unordered_map<std::string, T>>
	{
	public:
		std::unordered_map<std::string, T> operator()(const std::string &v)
		{
			YAML::Node node = YAML::Load(v);
			typename std::unordered_map<std::string, T> maps;
			std::stringstream ss;
			for (auto it = node.begin(); it != node.end(); ++it)
			{
				ss.str("");
				ss << it->second;
				maps.emplace(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
			}
			return maps;
		}
	};
	//std::unordered_map to std::string
	template <class T>
	class LexicalCast<std::unordered_map<std::string, T>, std::string>
	{
	public:
		std::string operator()(const std::unordered_map<std::string, T> &v)
		{
			YAML::Node node;
			for (auto &i : v)
			{
				node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	/**
	 * 	ConfigVarBase实现类
	**/
	template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
	class ConfigVar : public ConfigVarBase
	{
	public:
		using RWMutexType = RW_Mutex;
		using ptr = std::shared_ptr<ConfigVar>;
		using on_change_cb = std::function<void(const T &old_val, const T &new_val)>;
		ConfigVar(const std::string &name, const T &default_value, const std::string &des = "")
			: ConfigVarBase(name, des), _temp_val(default_value)
		{
		}
		std::string toString() override
		{
			try
			{
				RWMutexType::ReadLock lock(_mutex);
				return ToStr()(_temp_val);
			}
			catch (std::exception &e)
			{
				ROUTN_LOG_ERROR(g_logger) << "ConfigVar::toString exception" << e.what()
												  << " convert: " << typeid(_temp_val).name() << " to string";
			}
			return "";
		}
		bool fromString(const std::string &val) override
		{
			try
			{
				setValue(FromStr()(val));
			}
			catch (std::exception &e)
			{
				ROUTN_LOG_ERROR(g_logger) << "ConfigVar::toString exception" << e.what()
												  << " convert: " << typeid(_temp_val).name();
			}
			return false;
		}

		const T getValue()
		{
			RWMutexType::ReadLock lock(_mutex);
			return _temp_val;
		}

		void setValue(const T &v)
		{
			{//中括号的作用：读写锁分离，离开此括号说明线程完成了读操作，读锁析构，才可以写，否则会出现读锁还未构造，却已经开始写操作
				RWMutexType::ReadLock rlock(_mutex);
				if (v == _temp_val)
					return;
				for (auto it = _m_callbacks.begin(); it != _m_callbacks.end(); ++it)
				{
					it->second(_temp_val, v);
				}
			}
			RWMutexType::WriteLock wlock(_mutex);
			_temp_val = v;
		}
		std::string getTypeName() const override { return typeid(T).name(); }

		uint64_t addListener(on_change_cb callback)
		{
			static uint64_t s_fun_id = 0;
			RWMutexType::WriteLock lock(_mutex);
			++s_fun_id;
			_m_callbacks[s_fun_id] = callback;
			return s_fun_id;
		}

		void delListener(uint64_t key)
		{
			RWMutexType::WriteLock lock(_mutex);
			_m_callbacks.erase(key);
		}
		on_change_cb getListener(uint64_t key)
		{
			RWMutexType::ReadLock lock(_mutex);
			auto it = _m_callbacks.find(key);
			return it == _m_callbacks.end() ? nullptr : it->second;
		}

	private:
		T _temp_val;
		RWMutexType _mutex;

		//变更回调函数组, uint64_t key, 要求唯一， 一般用hash
		std::map<uint64_t, on_change_cb> _m_callbacks;
	};

	//全局实例类Config
	class Config
	{
	public:
		using ConfigVarMap = std::unordered_map<std::string, ConfigVarBase::ptr>;
		using RWMutexType = RW_Mutex;
		template <class T>
		static typename ConfigVar<T>::ptr Lookup(const std::string &name)
		{
			RWMutexType::ReadLock lock(GetMutex());
			auto it = GetDatas().find(name);
			if (it == GetDatas().end())
			{
				return nullptr;
			}
			return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
		}

		template <class T>
		static typename ConfigVar<T>::ptr Lookup(const std::string &name,
												 const T &default_val, const std::string &des = "")
		{
			RWMutexType::WriteLock lock(GetMutex());
			auto it = GetDatas().find(name);
			if (it != GetDatas().end())
			{
				auto temp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
				if (temp)
				{
					ROUTN_LOG_INFO(g_logger) << "Lookup name = " << name << " exist";
					return temp;
				}
				else
				{
					ROUTN_LOG_ERROR(g_logger) << "Lookup name = " << name << " exist but not: "
													  << typeid(T).name() << " the real type: " << it->second->getTypeName() << " value: "
													  << it->second->toString();
					return nullptr;
				}
			}
			if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos)
			{
				ROUTN_LOG_ERROR(g_logger) << "Lookup name invalid = " << name;
				throw std::invalid_argument(name);
			}
			typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_val, des));
			GetDatas()[name] = v;
			return v;
		}

		static void LoadFromYaml(const YAML::Node &root);
		
		static void LoadFromConfDir(const std::string& path);

		static ConfigVarBase::ptr LookupBase(const std::string &name);

		static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

	private:
		static ConfigVarMap &GetDatas()
		{
			static ConfigVarMap _m_datas;
			return _m_datas;
		}
		static RWMutexType &GetMutex()
		{
			static RWMutexType s_mutex;
			return s_mutex;
		}
	};

};

#endif

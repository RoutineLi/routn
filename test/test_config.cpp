/*************************************************************************
	> File Name: test_config.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月26日 星期三 17时02分39秒
 ************************************************************************/

#include "../src/Config.h"
#include "../src/Env.h"
#include "../src/Log.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

Routn::ConfigVar<int>::ptr g_int_value_config =
	 Routn::Config::Lookup("system.port", (int)8080, "system port");

//Routn::ConfigVar<float>::ptr g_int_valuex_config =
//	 Routn::Config::Lookup("system.port", (float)8080, "system port");

Routn::ConfigVar<float>::ptr g_float_value_config = 
	 Routn::Config::Lookup("system.value", (float)10.2f, "system value");

Routn::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config = 
	 Routn::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int vec");

Routn::ConfigVar<std::list<int> >::ptr g_int_list_value_config = 
	 Routn::Config::Lookup("system.int_list", std::list<int>{1, 2}, "system int list");

Routn::ConfigVar<std::set<int> >::ptr g_int_set_value_config = 
	 Routn::Config::Lookup("system.int_set", std::set<int>{1, 2}, "system int set");

Routn::ConfigVar<std::unordered_set<int> >::ptr g_int_hash_set_value_config = 
	 Routn::Config::Lookup("system.int_hash_set", std::unordered_set<int>{1, 2}, "system int hash_set");

Routn::ConfigVar<std::map<std::string, int> >::ptr g_int_map_value_config = 
	 Routn::Config::Lookup("system.int_map", std::map<std::string, int>{{"k", 2}}, "system int_map");

Routn::ConfigVar<std::unordered_map<std::string, int> >::ptr g_int_hash_map_value_config = 
	 Routn::Config::Lookup("system.int_hash_map", std::unordered_map<std::string, int>{{"k", 2}}, "system int hash_map");

void test_config(){
	ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
#define XX(g_var, name, prefix) \
	{\
		auto &v = g_var->getValue();\
		for(auto i : v){\
			ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << #prefix " " #name ": " << i;\
		}\
		ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << #prefix " " #name " yaml: \n" << g_var->toString();\
	}

#define XX_M(g_var, name, prefix) \
	{\
		auto &v = g_var->getValue();\
		for(auto i : v){\
			ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << #prefix " " #name ": { " \
			 << i.first << " -> " << i.second << " }";\
		}\
		ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << #prefix " " #name " yaml: \n" << g_var->toString();\
	}

	XX(g_int_vec_value_config, int_vec, before)
	XX(g_int_list_value_config, int_list, before)
	XX(g_int_set_value_config, int_set, before)
	XX(g_int_hash_set_value_config, int_hash_set, before)
	XX_M(g_int_map_value_config, int_map, before)
	XX_M(g_int_hash_map_value_config, int_hash_map, before)

	YAML::Node root = YAML::LoadFile("/home/rotun-li/routn/bin/conf/test.yml");
	Routn::Config::LoadFromYaml(root);

	
	XX(g_int_vec_value_config, int_vec, after)
	XX(g_int_list_value_config, int_list, after)
	XX(g_int_set_value_config, int_set, after)
	XX(g_int_hash_set_value_config, int_hash_set, after)
	XX_M(g_int_map_value_config, int_map, before)
	XX_M(g_int_hash_map_value_config, int_hash_map, after)

#undef XX
#undef XX_M
}

void print_yaml(const YAML::Node& node, int level){
	if(node.IsScalar()){
		ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() <<  " - " << node.Tag();
	}else if(node.IsNull()){
		ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - " << level;
	}else if(node.IsMap()){
		for(auto it = node.begin(); it != node.end(); ++it){
			ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
			print_yaml(it->second, level + 1);
		}
	}else if(node.IsSequence()){
		for(size_t i = 0; i < node.size(); ++i){
			ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
			print_yaml(node[i], level + 1);
		}		
	}
}

void test_yaml(){
	YAML::Node root = YAML::LoadFile("/home/rotun-li/routn/bin/conf/test.yml");
	print_yaml(root, 0);
	/**
	Routn::Logger::ptr logger(new Routn::Logger("test_yaml"));
	logger->addAppender(Routn::LogAppender::ptr(new Routn::FileAppender("/home/rotun-li/routn/log/testyaml.txt")));
	ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << root;
	ROUTN_LOG_INFO(logger) << root;
	**/
}
class Person{
public:
	Person(){

	}
	Person(const std::string &name, int n, bool m)
		: _name(name)
		, _age(n)
		, _sex(m)
	{}
	std::string _name;
	int _age = 0;
	bool _sex = 0;
	std::string toString() const{
		std::stringstream ss;
		ss << "{Person name = " << _name 
			<< " age = " << _age
			<< " sex = " << (_sex == true ? "female" : "male") << " }";
		return ss.str(); 
	}
	bool operator==(const Person& oth) const{
		return _name == oth._name
			&& _sex == oth._sex
			&& _age == oth._age;
	}
};

Routn::ConfigVar<Person>::ptr g_person = Routn::Config::Lookup("class.person", Person("jxb", 21, false), "system person");
Routn::ConfigVar<std::map<std::string, Person> >::ptr g_persons = 
		Routn::Config::Lookup("class.map", std::map<std::string, Person>(), "system person map");
namespace Routn{
	template<>
	class LexicalCast<std::string, Person>{
	public:
		Person operator()(const std::string& v){
			YAML::Node node = YAML::Load(v);
			Person p;
			p._name = node["name"].as<std::string>();
			p._age = node["age"].as<int>();
			if(node["sex"].as<std::string>() == "male"){
				p._sex = false;
			}else{
				p._sex = true;
			}
			return p;
		}
	};
	
	template<>
	class LexicalCast<Person, std::string>{
	public:
		std::string operator()(const Person& p){
			YAML::Node node;
			node["name"] = p._name;
			node["age"] = p._age;
			if(p._sex == true)
				node["sex"] = "female";
			else node["sex"] = "male";
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};
}

void test_class(){
	ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();
	g_person->addListener([](const Person& old_val, const Person& new_val){
		ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << "old_value = " << old_val.toString()
				<< " new_value = " << new_val.toString();
	});
	YAML::Node root = YAML::LoadFile("/home/rotun-li/routn/bin/conf/test.yml");
	Routn::Config::LoadFromYaml(root);

	ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
}

void test_loadconf(){
	Routn::Config::LoadFromConfDir("conf");
}

void test_log(){
	std::cout << Routn::LoggerMgr::GetInstance()->toYamlString() << std::endl;
	static Routn::Logger::ptr system_log = ROUTN_LOG_NAME("system");
	YAML::Node root = YAML::LoadFile("/home/rotun-li/routn/bin/conf/log.yml");
	Routn::Config::LoadFromYaml(root);
	std::cout << "================" << std::endl;
	//std::cout << Routn::LoggerMgr::GetInstance()->toYamlString();
	std::cout << std::endl;
	ROUTN_LOG_INFO(system_log) << "hello system";
	system_log->setFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%c]%T[%p]%T%f: %l%T%m%n");
	ROUTN_LOG_INFO(system_log) << "hello system";

}

int main(int argc, char** argv){

	//ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << g_int_value_config->getValue();
	//ROUTN_LOG_INFO(ROUTN_LOG_ROOT()) << g_float_value_config->toString();

	//test_config();
	//std::cout << "------------------------------------------------------------------------------------\n";
	//test_class();
	Routn::EnvMgr::GetInstance()->init(argc, argv);
	test_loadconf();
	
	return 0;
}
	



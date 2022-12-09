/*************************************************************************
	> File Name: test_env.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月27日 星期日 03时31分40秒
 ************************************************************************/

#include "../src/Env.h"
#include <fstream>
#include <iostream>
#include <unistd.h>
struct A{
	A(){
		std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
		std::string content;
		content.resize(4096);

		ifs.read(&content[0], content.size());
		content.resize(ifs.gcount());

		for(size_t i = 0; i < content.size(); ++i){
			std::cout << i << " - " << content[i] << " - " << (int)content[i] << std::endl;
		}

		std::cout << content << std::endl;
	}
};

A a;

int main(int argc, char** argv){
	Routn::EnvMgr::GetInstance()->addHelpInfo("d", "run as daemon");
	Routn::EnvMgr::GetInstance()->addHelpInfo("s", "run as default");
	Routn::EnvMgr::GetInstance()->addHelpInfo("p", "print help");
	
	if(!Routn::EnvMgr::GetInstance()->init(argc, argv)){
		Routn::EnvMgr::GetInstance()->printHelp();
		return 0;
	}

	std::cout << "exe = " << Routn::EnvMgr::GetInstance()->getExe() << std::endl;
	std::cout << "cwd = " << Routn::EnvMgr::GetInstance()->getCwd() << std::endl;
	
	std::cout << "path = " << Routn::EnvMgr::GetInstance()->getEnv("path", "xxx") << std::endl;
//	std::cout << "set env = " << Routn::EnvMgr::GetInstance()->setEnv()

	if(Routn::EnvMgr::GetInstance()->hasKey("p")){
		Routn::EnvMgr::GetInstance()->printHelp();
	}

	return 0;
}


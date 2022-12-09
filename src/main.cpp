/*************************************************************************
	> File Name: main.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月28日 星期一 11时28分05秒
 ************************************************************************/

#include "../src/Routn.h"
#include "../src/Application.h"

#include <ctime>
#include <cstdlib>

int main(int argc, char** argv){
	setenv("TZ", ":/etc/localtime", 1);
	tzset();
	srand(time(0));
    Routn::Application app;
    if(app.init(argc, argv)) {
        return app.run();
    }
    return 0;
}


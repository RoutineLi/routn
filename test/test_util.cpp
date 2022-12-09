/*************************************************************************
	> File Name: test_util.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月30日 星期日 14时19分17秒
 ************************************************************************/

#include "../src/Routn.h"

Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();


void test_assert(){
	ROUTN_LOG_INFO(g_logger) << Routn::BacktraceToString(10, 0, "   ");
	ROUTN_ASSERT_ARG(0 == 1, "abcdef xx");
}

int main(int argc, char** argv){
	
	test_assert();

    return 0;
}




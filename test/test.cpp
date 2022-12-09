/*************************************************************************
	> File Name: test.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月24日 星期一 19时49分44秒
 ************************************************************************/

#include <iostream>
#include <cstdio>
#include "../src/Log.h"

using namespace std;
using namespace Routn;

int main(int argc, char** argv){
    Routn::Logger::ptr logger(new Routn::Logger("test"));
    logger->addAppender(Routn::LogAppender::ptr(new Routn::FileAppender("./log/testlog.txt")));
    logger->addAppender(Routn::LogAppender::ptr(new Routn::StdoutAppender));

    //Routn::LogEvent::ptr event(new Routn::LogEvent(__FILE__, __LINE__, 0, (uint32_t)Routn::GetThreadId(), 2, time(0)));

    //event->getContent() << "Hello Routn_log !";

    //logger->log(Routn::LogLevel::DEBUG, event);
    
    ROUTN_LOG_INFO(logger) << "Routn_log macro test [info]";
    ROUTN_LOG_ERROR(logger) << "Routn_log macro test [ERROR]";
    ROUTN_LOG_FATAL(logger) << "Routn_log macro test [FATAL]";

    ROUTN_LOG_FMT_INFO(logger, "Routn_log_%s macro test [INFO]", "format");

    auto l = Routn::LoggerMgr::GetInstance()->getLogger("test");
    ROUTN_LOG_INFO(l) << "test1";

    return 0;
}

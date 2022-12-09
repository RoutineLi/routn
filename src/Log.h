/*************************************************************************
	> File Name: Log.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月22日 星期六 21时29分02秒
 ************************************************************************/

#ifndef _LOG_H
#define _LOG_H


#include "Util.h"
#include "Thread.h"
#include "Singleton.h"

#include <string>
#include <cstring>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <stdarg.h>
#include <ctime>
#include <cstdio>
#include <memory>
#include <list>
#include <functional>
#include <unordered_map>

#define ROUTN_LOG_LEVEL(logger, level)                                                                               \
    if (logger->getLevel() <= level)                                                                                 \
    Routn::LogEventWrap(Routn::LogEvent::ptr(new Routn::LogEvent(logger,                                             \
                                                                 level, __FILE__, __LINE__, 0, Routn::GetThreadId(), \
                                                                 Routn::GetFiberId(), time(0), Routn::Thread::GetName())))                     \
        .getSS()

#define ROUTN_LOG_DEBUG(logger) ROUTN_LOG_LEVEL(logger, Routn::LogLevel::DEBUG)
#define ROUTN_LOG_INFO(logger) ROUTN_LOG_LEVEL(logger, Routn::LogLevel::INFO)
#define ROUTN_LOG_WARN(logger) ROUTN_LOG_LEVEL(logger, Routn::LogLevel::WARN)
#define ROUTN_LOG_ERROR(logger) ROUTN_LOG_LEVEL(logger, Routn::LogLevel::ERROR)
#define ROUTN_LOG_FATAL(logger) ROUTN_LOG_LEVEL(logger, Routn::LogLevel::FATAL)

#define ROUTN_LOG_FMT_LEVEL(logger, level, fmt, ...)                                                                 \
    if (logger->getLevel() <= level)                                                                                 \
    Routn::LogEventWrap(Routn::LogEvent::ptr(new Routn::LogEvent(logger,                                             \
                                                                 level, __FILE__, __LINE__, 0, Routn::GetThreadId(), \
                                                                 Routn::GetFiberId(), time(0), Routn::Thread::GetName())))                     \
        .getEvent()                                                                                                  \
        ->format(fmt, __VA_ARGS__)

#define ROUTN_LOG_FMT_DEBUG(logger, fmt, ...) ROUTN_LOG_FMT_LEVEL(logger, Routn::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define ROUTN_LOG_FMT_ERROR(logger, fmt, ...) ROUTN_LOG_FMT_LEVEL(logger, Routn::LogLevel::ERROR, fmt, __VA_ARGS__)
#define ROUTN_LOG_FMT_FATAL(logger, fmt, ...) ROUTN_LOG_FMT_LEVEL(logger, Routn::LogLevel::FATAL, fmt, __VA_ARGS__)
#define ROUTN_LOG_FMT_INFO(logger, fmt, ...) ROUTN_LOG_FMT_LEVEL(logger, Routn::LogLevel::INFO, fmt, __VA_ARGS__)
#define ROUTN_LOG_FMT_WARN(logger, fmt, ...) ROUTN_LOG_FMT_LEVEL(logger, Routn::LogLevel::WARN, fmt, __VA_ARGS__)

#define ROUTN_LOG_ROOT()  Routn::LoggerMgr::GetInstance()->getRoot()
#define ROUTN_LOG_NAME(name)  Routn::LoggerMgr::GetInstance()->getLogger(name)

namespace Routn
{
    using MutexType = Routn::SpinLock;
    class Logger;
    class LogManager;

    enum class LogLevel
    {
        UNKNOWN = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5

    };

    //将日志级别转成文本输出
    static const char *LevelToString(LogLevel level);
    //将文本转换成日志级别
    static LogLevel FromString(const std::string &str);

    class LogEvent
    {
    public:
        using ptr = std::shared_ptr<LogEvent>;
        LogEvent(std::shared_ptr<Logger> logger, LogLevel level, const char *file, int32_t m_line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time, const std::string& thread_name);
        ~LogEvent();
        inline const char *getFile() const
        {
            return pFile;
        }
        inline int32_t getLine() const
        {
            return i32Line;
        }
        inline uint32_t getElapse() const
        {
            return ui32Elapse;
        }
        inline uint32_t getThreadId() const
        {
            return ui32ThreadId;
        }
        inline uint32_t getFiberId() const
        {
            return ui32FiberId;
        }
        inline uint64_t getTime() const
        {
            return ui64Time;
        }
        inline std::string getContentStr() const
        {
            return _content.str();
        }
        inline std::stringstream &getContent()
        {
            return _content;
        }
        inline std::shared_ptr<Logger> &getLogger()
        {
            return _logger;
        }
        inline LogLevel getLevel()
        {
            return _level;
        }
        inline const std::string& getThreadName() const
        {
            return _threadName;
        }

        void format(const char *fmt, ...);
        void format(const char *fmt, va_list arg);

    private:
        const char *pFile;
        int32_t i32Line;            //行号
        uint32_t ui32ThreadId;      //线程id
        uint32_t ui32Elapse;        //程序启动到现在的毫秒数
        uint32_t ui32FiberId;       //协程id
        uint64_t ui64Time;          //时间戳
        std::stringstream _content; //内容
        std::shared_ptr<Logger> _logger;
        LogLevel _level;
        std::string _threadName;
    };

    class LogEventWrap
    {
    public:
        LogEventWrap(LogEvent::ptr e);
        ~LogEventWrap();
        LogEvent::ptr &getEvent();
        std::stringstream &getSS();

    private:
        LogEvent::ptr _event;
    };

    /**
 * -----日志格式器-----
 * %m -- 消息体
 * %p -- 日志级别
 * %r -- 启动时间
 * %c -- 日志名称
 * %t -- 线程id
 * %n -- 回车换行
 * %d -- 日期时间
 * %f -- 文件名称
 * %l -- 行号
 * %T -- 制表符
 * %F -- 协程id
**/
    class LogFormatter
    {
        friend class Logger;

    public:
        using ptr = std::shared_ptr<LogFormatter>;
        LogFormatter(const std::string &pattern);
        std::string format(std::shared_ptr<Logger> logger, LogLevel level, LogEvent::ptr event);

    public:
        class FormatItem
        {
        public:
            using ptr = std::shared_ptr<FormatItem>;
            FormatItem(const std::string &fmt = "")
                : _format(fmt)
            {
            }
            virtual ~FormatItem() {}
            virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) = 0;

        private:
            std::string _format;
        };
        void init(); //执行日志的解析
        bool isError() const { return _error; }
        const std::string getPattern() const { return _pattern; }

    private:
        std::string _pattern;
        std::vector<FormatItem::ptr> _Vec_items;
        bool _error = false;
    };

    /**
 *  日志输出地 
**/

    class LogAppender
    {
        friend class Logger;

    public:
        using ptr = std::shared_ptr<LogAppender>;
        LogAppender();
        virtual ~LogAppender();
        virtual std::string toYamlString() = 0;
        virtual void log(std::shared_ptr<Logger> logger, LogLevel level, LogEvent::ptr event) = 0;
        void setFormatter(LogFormatter::ptr val);
        LogFormatter::ptr getFormatter();
        void setLevel(LogLevel level) { _level = level; }
        void setTimeAsNeeded(bool v) { _time_as_name_needed = v; }
        bool isTimeAsNeeded() const { return _time_as_name_needed; }
        LogLevel getLevel() const { return _level; }

    protected:
        LogLevel _level = LogLevel::DEBUG;
        bool _b_hasFormatter = false;
        MutexType _mutex;
        LogFormatter::ptr _ptr_formatter;
        bool _time_as_name_needed = true; ///文件名后缀是否加上创建日期
    };

    class StdoutAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<StdoutAppender>;
        StdoutAppender();
        ~StdoutAppender();
        std::string toYamlString() override;
        void log(std::shared_ptr<Logger> logger, LogLevel level, LogEvent::ptr event);
    };

    class FileAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<FileAppender>;
        FileAppender(const std::string &filename, bool value = false);
        ~FileAppender();
        std::string toYamlString() override;
        void log(std::shared_ptr<Logger> logger, LogLevel level, LogEvent::ptr event);
        /**
             * @brief 重新打开日志文件
             * @return 成功返回true
             */
        bool reopen();
        void resetFileName();

    private:
        /// 文件路径
        std::string _filename;
        /// 文件流
        std::ofstream _ofs_stream;
        /// 上次重新打开时间
        uint64_t _lastTime = 0;
    };
    /*
 *  日志输出器
 */

    class Logger : public std::enable_shared_from_this<Logger>
    {
        friend class LogManager;

    public:
        using ptr = std::shared_ptr<Logger>;
        Logger(const std::string &name = "root");
        ~Logger() {}
        void log(LogLevel level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        inline LogLevel getLevel() const { return _level; }
        inline void setLevel(LogLevel val) { _level = val; }
        inline const std::string &getName() const { return _logname; }

        void setFormatter(LogFormatter::ptr val);
        void setFormatter(const std::string &val);

        LogFormatter::ptr getFormatter();
        std::string toYamlString();

        void clearAppenders();
        void addAppender(LogAppender::ptr addevent);
        void delAppender(LogAppender::ptr delevent);

    protected:
        std::string _logname;                        //日志名称
        LogLevel _level;                             //日志级别
        std::list<LogAppender::ptr> _list_appenders; //Appender集合
        LogFormatter::ptr _formatter;
        MutexType _mutex;
        Logger::ptr _root;
    };

    class LogManager
    {
    public:
        LogManager();
        Logger::ptr getLogger(const std::string &name);
        void init();
        Logger::ptr getRoot() const { return _logger; }
        std::string toYamlString();

    private:
        std::unordered_map<std::string, Logger::ptr> _m_loggers;
        Logger::ptr _logger;
        MutexType _mutex;
    };
    using LoggerMgr = Routn::Singleton<LogManager>;
}
#endif

/*************************************************************************
	> File Name: Log.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月22日 星期六 22时05分45秒
 ************************************************************************/

#include "Log.h"
#include "Config.h"

namespace Routn
{

    const char *LevelToString(LogLevel level)
    {
        switch (level)
        {
#define XX(name)         \
    case LogLevel::name: \
        return #name;    \
        break;

            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);

#undef XX
        default:
            return "UNKOWN";
            break;
        }
    }

    LogLevel FromString(const std::string &str)
    {
#define XX(level, v)            \
    if (str == #v)              \
    {                           \
        return LogLevel::level; \
    }
        XX(DEBUG, debug);
        XX(INFO, info);
        XX(WARN, warn);
        XX(ERROR, error);
        XX(FATAL, fatal);

        XX(DEBUG, DEBUG);
        XX(INFO, INFO);
        XX(WARN, WARN);
        XX(ERROR, ERROR);
        XX(FATAL, FATAL);
        return LogLevel::UNKNOWN;
#undef XX
    }

    /*
            const char* pFile;
            int32_t i32Line;                   //行号
            uint32_t ui32ThreadId;             //线程id
            uint32_t ui32Elapse;               //程序启动到现在的毫秒数 
            uint32_t ui32FiberId;              //协程id
            uint64_t ui64Time;                 //时间戳
            std::stringstream _content;        //内容
    */

    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel level, const char *file, int32_t m_line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time, const std::string& thread_name)
        : pFile(file), i32Line(m_line), ui32ThreadId(thread_id), ui32Elapse(elapse), ui32FiberId(fiber_id), ui64Time(time), _logger(logger), _level(level), _threadName(thread_name)
    {
    }

    LogEvent::~LogEvent()
    {
    }

    void LogEvent::format(const char *fmt, ...)
    {
        va_list val;
        va_start(val, fmt);
        format(fmt, val);
        va_end(val);
    }

    void LogEvent::format(const char *fmt, va_list arg)
    {
        char *buff = nullptr;
        int len = vasprintf(&buff, fmt, arg);
        if (len != -1)
        {
            _content << std::string(buff, len);
            free(buff);
        }
    }

    LogEventWrap::LogEventWrap(LogEvent::ptr e)
        : _event(e)
    {
    }

    LogEventWrap::~LogEventWrap()
    {
        if (_event)
        {
            _event->getLogger()->log(_event->getLevel(), _event);
        }
    }

    LogEvent::ptr &LogEventWrap::getEvent()
    {
        return _event;
    }

    std::stringstream &LogEventWrap::getSS()
    {
        return _event->getContent();
    }

    /*--------------------------------------------Logger--------------------------------------------------------*/

    Logger::Logger(const std::string &name)
        : _logname(name), _level(LogLevel::DEBUG)
    {
        _formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f: %l%T%m%n"));
    }

    void Logger::log(LogLevel level, LogEvent::ptr event)
    {
        if (level >= _level)
        {
            auto self = shared_from_this();
            MutexType::Lock lock(_mutex);
            if (!_list_appenders.empty())
            {
                for (auto &x : _list_appenders)
                {
                    x->log(self, level, event);
                }
            }
            else if (_root)
            {
                _root->log(level, event);
            }
        }
    }

    void Logger::debug(LogEvent::ptr event)
    {
        log(LogLevel::DEBUG, event);
    }

    void Logger::info(LogEvent::ptr event)
    {
        log(LogLevel::INFO, event);
    }

    void Logger::warn(LogEvent::ptr event)
    {
        log(LogLevel::WARN, event);
    }

    void Logger::fatal(LogEvent::ptr event)
    {
        log(LogLevel::FATAL, event);
    }

    void Logger::error(LogEvent::ptr event)
    {
        log(LogLevel::ERROR, event);
    }

    void Logger::addAppender(LogAppender::ptr addevent)
    {
        MutexType::Lock lock(_mutex);
        if (!addevent->getFormatter())
        {
            MutexType::Lock _lock(addevent->_mutex);
            addevent->_ptr_formatter = _formatter;
        }
        _list_appenders.push_back(addevent);
    }

    void Logger::delAppender(LogAppender::ptr delevent)
    {
        MutexType::Lock lock(_mutex);
        for (auto it = _list_appenders.begin(); it != _list_appenders.end(); ++it)
        {
            if (*it == delevent)
            {
                _list_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppenders()
    {
        MutexType::Lock lock(_mutex);
        _list_appenders.clear();
    }

    void Logger::setFormatter(LogFormatter::ptr val)
    {
        MutexType::Lock lock(_mutex);
        _formatter = val;
        for (auto &i : _list_appenders)
        {
            MutexType::Lock _lock(i->_mutex);
            if (!i->_b_hasFormatter)
            {
                i->_ptr_formatter = _formatter;
            }
        }
    }

    void Logger::setFormatter(const std::string &val)
    {
        Routn::LogFormatter::ptr new_val(new Routn::LogFormatter(val));
        if (new_val->isError())
        {
            std::cout << "Logger setFormatter name = " << _logname
                      << "value = " << val << "invalid formatter" << std::endl;
            return;
        }
        setFormatter(new_val);
    }

    LogFormatter::ptr Logger::getFormatter()
    {
        MutexType::Lock lock(_mutex);
        return this->_formatter;
    }

    std::string Logger::toYamlString()
    {
        MutexType::Lock lock(_mutex);
        YAML::Node node;
        node["name"] = _logname;
        if (_level != LogLevel::UNKNOWN)
        {
            node["level"] = LevelToString(_level);
        }
        if (_formatter)
        {
            node["formatter"] = _formatter->getPattern();
            //std::cout << "formatters" << _formatter->getPattern();
        }

        for (auto &i : _list_appenders)
        {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
            //std::cout << "listappenders" << i->toYamlString() << std::endl;
        }
        std::stringstream ss;
        ss << node;
        //std::cout << "[logger]\n" << ss.str() << std::endl;
        return ss.str();
    }
    /*--------------------------------------------LogAppender------------------------------------------------------------------------------------*/

    LogAppender::LogAppender()
    {
    }

    LogAppender::~LogAppender()
    {
    }

    void LogAppender::setFormatter(LogFormatter::ptr val)
    {
        MutexType::Lock lock(_mutex);
        _ptr_formatter = val;
        if (_ptr_formatter)
        {
            _b_hasFormatter = true;
        }
        else
        {
            _b_hasFormatter = false;
        }
    }

    LogFormatter::ptr LogAppender::getFormatter()
    {
        MutexType::Lock lock(_mutex);
        return _ptr_formatter;
    }

    FileAppender::FileAppender(const std::string &filename, bool value)
        : LogAppender(), _filename(filename)
    {
        setTimeAsNeeded(value);
        reopen();
    }

    FileAppender::~FileAppender()
    {
    }

    void FileAppender::resetFileName()
    {
        //为log文件名称加上时间戳方便记录

        struct tm st_tm;
        time_t t_time = (time_t)time(0);
        localtime_r(&t_time, &st_tm);
        std::string t_format = "%Y-%m-%d %H:%M:%S";
        char buff[64] = {0};
        strftime(buff, sizeof(buff), t_format.c_str(), &st_tm);
        std::string temp = "-";
        temp.append(buff);
        int pos = _filename.find('.');
        _filename.insert(pos, temp);
    }

    void FileAppender::log(Logger::ptr logger, LogLevel level, LogEvent::ptr event)
    {
        MutexType::Lock lock(_mutex);    
        /*uint64_t now = event->getTime();
        if(now != _lastTime){
            reopen();
            _lastTime = now;
        }*/
        if (level >= _level)
        {
            _ofs_stream << _ptr_formatter->format(logger, level, event);
        }
    }

    std::string FileAppender::toYamlString()
    {
        MutexType::Lock lock(_mutex);
        YAML::Node node;
        node["type"] = "FileAppender";
        node["file"] = _filename;
        if (_level != LogLevel::UNKNOWN)
        {
            node["level"] = LevelToString(_level);
        }
        if (_ptr_formatter && _b_hasFormatter)
        {
            node["formatter"] = _ptr_formatter->getPattern();
        }
        if (isTimeAsNeeded())
        {
            node["timeasname"] = "true";
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    bool FileAppender::reopen()
    {
        MutexType::Lock lock(_mutex);
        if (_ofs_stream)
        {
            _ofs_stream.close();
        }
        if (this->isTimeAsNeeded())
        {
            this->resetFileName();
        }
        _ofs_stream.open(_filename, std::ios::app | std::ios::out);
        return !!_ofs_stream;
    }

    StdoutAppender::StdoutAppender()
        : LogAppender()
    {
    }

    StdoutAppender::~StdoutAppender()
    {
    }

    std::string StdoutAppender::toYamlString()
    {
        MutexType::Lock lock(_mutex);
        YAML::Node node;
        node["type"] = "StdoutAppender";
        if (_level != LogLevel::UNKNOWN)
        {
            node["level"] = LevelToString(_level);
        }
        if (_b_hasFormatter && _ptr_formatter)
        {
            node["formatter"] = _ptr_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void StdoutAppender::log(std::shared_ptr<Logger> logger, LogLevel level, LogEvent::ptr event)
    {
        if (level >= _level)
        {
            MutexType::Lock lock(_mutex);
            std::cout << _ptr_formatter->format(logger, level, event);
        }
    }

    /*-----------------------------------------------LogFormatter------------------------------------------------------------------------------------*/
    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &fmt = "")
            : FormatItem(fmt)
        {
        }
        virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            os << event->getContentStr();
        }
    };
    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &fmt = "")
            : FormatItem(fmt)
        {
        }
        virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            os << LevelToString(level);
        }
    };
    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &fmt = "")
            : FormatItem(fmt)
        {
        }
        virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            os << event->getElapse();
        }
    };
    class LogNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        LogNameFormatItem(const std::string &fmt = "")
            : FormatItem(fmt)
        {
        }
        void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            os << event->getLogger()->getName();
        }
    };
    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &fmt = "")
            : FormatItem(fmt)
        {
        }
        virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            os << event->getThreadId();
        }
    };
    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &fmt = "")
            : FormatItem(fmt)
        {
        }
        virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            os << event->getFiberId();
        }
    };
    class ThreadNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadNameFormatItem(const std::string &fmt = "")
            : FormatItem(fmt)
        {
        }
        virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            os << event->getThreadName();
        }
    };
    class GetLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        GetLineFormatItem(const std::string &fmt = "")
            : FormatItem(fmt)
        {
        }
        virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            os << std::endl;
        }
    };
    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string &str)
            : FormatItem(str),
              _string(str)
        {
        }
        virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            os << _string;
        }

    private:
        std::string _string;
    };
    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        TabFormatItem(const std::string &str = "")
            : FormatItem(str)
        {
        }
        virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            os << "\t";
        }
    };
    class DateFormatItem : public LogFormatter::FormatItem
    {
    public:
        DateFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
            : _format(format)
        {
            if (_format.empty())
            {
                _format = "%Y-%m-%d %H:%M:%S";
            }
        }
        virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            struct tm st_tm;
            time_t time = event->getTime();
            localtime_r(&time, &st_tm);
            char buff[64] = {0};
            strftime(buff, sizeof(buff), _format.c_str(), &st_tm);
            os << buff;
        }

    private:
        std::string _format;
    };
    class FilenameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FilenameFormatItem(const std::string &fmt = "")
            : FormatItem(fmt)
        {
        }
        virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            os << event->getFile();
        }
    };
    class LineNumberFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineNumberFormatItem(const std::string &fmt = "")
            : FormatItem(fmt)
        {
        }
        virtual void Format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel level, LogEvent::ptr event) override
        {
            os << event->getLine();
        }
    };
    LogFormatter::LogFormatter(const std::string &pattern)
        : _pattern(pattern)
    {
        init();
    }

    std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel level, LogEvent::ptr event)
    {
        std::stringstream ss;
        for (auto &i : _Vec_items)
        {
            i->Format(logger, ss, level, event);
        }
        return ss.str();
    }

    //%xxx %xxx{xxx} %%
    void LogFormatter::init()
    {
        //str, format, type
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        for (size_t i = 0; i < _pattern.size(); i++)
        {
            if (_pattern[i] != '%')
            {
                nstr.append(1, _pattern[i]);
                continue;
            }

            if ((i + 1) < _pattern.size() && _pattern[i + 1] == '%')
            {
                nstr.append(1, '%');
                continue;
            }

            int fmt_status = 0;
            size_t n = i + 1;
            size_t fmt_begin = 0;

            std::string fmt, str;
            while (n < _pattern.size())
            {
                if (!fmt_status && (!isalpha(_pattern[n]) && _pattern[n] != '{' && _pattern[n] != '}'))
                {
                    str = _pattern.substr(i + 1, n - i - 1);
                    break;
                }
                if (fmt_status == 0)
                {
                    if (_pattern[n] == '{')
                    {
                        str = _pattern.substr(i + 1, n - i - 1);
                        fmt_status = 1; //解析格式
                        fmt_begin = n;
                        ++n;
                        continue;
                    }
                }
                else if (fmt_status == 1)
                {
                    if (_pattern[n] == '}')
                    {
                        fmt = _pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 0;
                        ++n;
                        break;
                    }
                }
                ++n;
                if (n == _pattern.size())
                {
                    if (str.empty())
                    {
                        str = _pattern.substr(i + 1);
                    }
                }
            }
            if (fmt_status == 0)
            {
                if (!nstr.empty())
                {
                    vec.push_back({nstr, "", 0});
                    nstr.clear();
                }
                vec.push_back({str, fmt, 1});
                i = n - 1;
            }
            else if (fmt_status == 1)
            {
                std::cout << "pattern parse error: " << _pattern << " - " << _pattern.substr(i) << std::endl;
                _error = true;
                vec.push_back({"<<pattern_error>>", fmt, 0});
            }
        }

        if (!nstr.empty())
        {
            vec.push_back({nstr, "", 0});
        }

        static std::map<std::string,
                        std::function<FormatItem::ptr(const std::string)>>
            s_format_items = {
#define XX(str, C)                              \
    {                                           \
#str, [](const std::string &fmt)        \
        { return FormatItem::ptr(new C(fmt)); } \
    }
                XX(m, MessageFormatItem),
                XX(p, LevelFormatItem),
                XX(r, ElapseFormatItem),
                XX(c, LogNameFormatItem),
                XX(t, ThreadIdFormatItem),
                XX(n, GetLineFormatItem),
                XX(d, DateFormatItem),
                XX(f, FilenameFormatItem),
                XX(l, LineNumberFormatItem),
                XX(T, TabFormatItem),
                XX(F, FiberIdFormatItem),
                XX(N, ThreadNameFormatItem)
#undef XX
            };
        //str, format, type
        for (auto &i : vec)
        {
            if (std::get<2>(i) == 0)
            {
                _Vec_items.push_back({FormatItem::ptr(new StringFormatItem(std::get<0>(i)))});
            }
            else
            {
                auto it = s_format_items.find(std::get<0>(i));
                if (it == s_format_items.end())
                {
                    _Vec_items.push_back({FormatItem::ptr(new StringFormatItem("<< Error_format %" + std::get<0>(i) + ">>"))});
                    _error = true;
                }
                else
                {
                    _Vec_items.push_back({it->second(std::get<1>(i))});
                }
            }
            //std::cout << "{" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << "}" << std::endl;
        }
    }

    /*-----------------------------------------------LogManager------------------------------------------------------------------------------------*/

    struct St_LogAppenderDefine
    {
        int type = 0; //1 File 2 Stdout
        LogLevel level = LogLevel::UNKNOWN;
        std::string formatter;
        std::string file;
        bool timeasname = false;

        bool operator==(const St_LogAppenderDefine &oth) const
        {
            return type == oth.type &&
                   level == oth.level &&
                   formatter == oth.formatter &&
                   file == oth.file &&
                   timeasname == oth.timeasname;
        }
    };

    struct St_LogDefine
    {
        std::string name;
        LogLevel level;
        std::string formatter;
        std::vector<St_LogAppenderDefine> appenders;

        bool operator==(const St_LogDefine &oth) const
        {
            return name == oth.name && level == oth.level && formatter == oth.formatter && appenders == oth.appenders;
        }
        bool operator<(const St_LogDefine &oth) const
        {
            return name < oth.name;
        }
    };

    /**
     * Log模块配置文件自定义结构模板片特化
    **/

    template <>
    class LexicalCast<std::string, St_LogDefine>
    {
    public:
        St_LogDefine operator()(const std::string &v)
        {
            YAML::Node n = YAML::Load(v);
            St_LogDefine ld;
            if (!n["name"].IsDefined())
            {
                std::cout << "log config error: name is null, " << n
                          << std::endl;
                throw std::logic_error("log config name is null");
            }
            ld.name = n["name"].as<std::string>();
            //std::cout << ld.name << " debug1" << std::endl;
            ld.level = FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
            if (n["formatter"].IsDefined())
            {
                ld.formatter = n["formatter"].as<std::string>();

                //std::cout << ld.formatter << " debug1" << std::endl;
            }

            if (n["appenders"].IsDefined())
            {
                //std::cout << "==" << ld.name << " = " << n["appenders"].size() << std::endl;
                for (size_t x = 0; x < n["appenders"].size(); ++x)
                {
                    auto a = n["appenders"][x];
                    if (!a["type"].IsDefined())
                    {
                        std::cout << "log config error: appender type is null, " << a
                                  << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();

                    St_LogAppenderDefine lad;
                    if (type == "FileAppender")
                    {
                        lad.type = 1;
                        if (!a["file"].IsDefined())
                        {
                            std::cout << "log config error: fileappender file is null, " << a
                                      << std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();

                        //std::cout << lad.file << " debug1" << std::endl;
                        if (a["formatter"].IsDefined())
                        {
                            lad.formatter = a["formatter"].as<std::string>();

                            //std::cout << lad.formatter << " debug1" << std::endl;
                        }
                        if (a["timeasname"].IsDefined())
                        {
                            lad.timeasname = a["timeasname"].as<bool>();
                        }
                    }
                    else if (type == "StdoutAppender")
                    {
                        lad.type = 2;
                        if (a["formatter"].IsDefined())
                        {
                            lad.formatter = a["formatter"].as<std::string>();
                            //std::cout << lad.formatter << " debug1" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "log config error: appender type is invalid, " << a
                                  << std::endl;
                        continue;
                    }

                    ld.appenders.push_back(lad);
                }
            }
            return ld;
        }
    };
    template <>
    class LexicalCast<St_LogDefine, std::string>
    {
    public:
        std::string operator()(const St_LogDefine &i)
        {
            YAML::Node n;
            n["name"] = i.name;
            if (i.level != LogLevel::UNKNOWN)
            {
                n["level"] = LevelToString(i.level);
            }
            if (!i.formatter.empty())
            {
                n["formatter"] = i.formatter;
            }

            for (auto &a : i.appenders)
            {
                YAML::Node na;
                if (a.type == 1)
                {
                    na["type"] = "FileAppender";
                    na["file"] = a.file;
                }
                else if (a.type == 2)
                {
                    na["type"] = "StdoutAppender";
                }
                if (a.level != LogLevel::UNKNOWN)
                {
                    na["level"] = LevelToString(a.level);
                }

                if (!a.formatter.empty())
                {
                    na["formatter"] = a.formatter;
                }

                n["appenders"].push_back(na);
            }
            std::stringstream ss;
            ss << n;
            return ss.str();
        }
    };

    Routn::ConfigVar<std::set<St_LogDefine>>::ptr g_log_defines =
        Routn::Config::Lookup("logs", std::set<St_LogDefine>(), "logs config");

    struct LogIniter
    {
        LogIniter()
        {
            g_log_defines->addListener([](const std::set<St_LogDefine> &old_val, const std::set<St_LogDefine> &new_val)
                                       {
                                           ROUTN_LOG_INFO(g_logger) << "on logger config changed!";
                                           for (auto &i : new_val)
                                           {
                                               auto it = old_val.find(i);
                                               Routn::Logger::ptr logger;
                                               if (it == old_val.end())
                                               {
                                                   //新增logger
                                                   logger = ROUTN_LOG_NAME(i.name);
                                               }
                                               else
                                               {
                                                   if (!(i == *it))
                                                   {
                                                       //修改的logger
                                                       logger = ROUTN_LOG_NAME(i.name);
                                                   }
                                                   else
                                                   {
                                                       continue;
                                                   }
                                               }
                                               logger->setLevel(i.level);
                                               //std::cout << LevelToString(logger->getLevel()) << "****" << std::endl;
                                               if (!i.formatter.empty())
                                               {
                                                   logger->setFormatter(i.formatter);
                                               }

                                               logger->clearAppenders();
                                               for (auto &a : i.appenders)
                                               {
                                                   Routn::LogAppender::ptr ap;
                                                   if (a.type == 1)
                                                   {
                                                       ap.reset(new FileAppender(a.file, a.timeasname));
                                                   }
                                                   else if (a.type == 2)
                                                   {
                                                       ap.reset(new StdoutAppender);
                                                   }
                                                   ap->setLevel(a.level);
                                                   if (!a.formatter.empty())
                                                   {
                                                       LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                                                       //std::cout << fmt->getPattern() << " *****" << std::endl;

                                                       if (!fmt->isError())
                                                       {
                                                           ap->setFormatter(fmt);
                                                           //std::cout << ap->getFormatter()->getPattern() << " *****" << std::endl;
                                                       }
                                                       else
                                                       {
                                                           std::cout << "log.name=" << i.name << " appender type=" << a.type
                                                                     << " formatter=" << a.formatter << " is invalid" << std::endl;
                                                       }
                                                   }
                                                   logger->addAppender(ap);
                                                   //std::cout << "debug1:" << ap->toYamlString() << " \n***** " << std::endl;
                                               }
                                           }

                                           for (auto &i : old_val)
                                           {
                                               auto it = new_val.find(i);
                                               if (it == new_val.end())
                                               {
                                                   //删除logger
                                                   auto logger = ROUTN_LOG_NAME(i.name);
                                                   logger->setLevel((LogLevel)0);
                                                   logger->clearAppenders();
                                               }
                                           }
                                       });
        }
    };

    static LogIniter __log_init;

    LogManager::LogManager()
    {
        _logger.reset(new Logger);
        _logger->addAppender(LogAppender::ptr(new StdoutAppender)); /*使用单例模式默认控制台输出*/

        _m_loggers[_logger->_logname] = _logger;

        init();
    }

    void LogManager::init()
    {
    }

    Logger::ptr LogManager::getLogger(const std::string &name)
    {
        MutexType::Lock lock(_mutex);
        auto it = _m_loggers.find(name);

        if (it != _m_loggers.end())
        {
            return it->second;
        }

        Logger::ptr logger(new Logger(name));
        //std::cout << logger->getName() << std::endl;
        logger->_root = _logger;
        _m_loggers[name] = logger;
        //std::cout << _logger->getName() << std::endl;
        return logger;
    }

    std::string LogManager::toYamlString()
    {
        MutexType::Lock lock(_mutex);
        YAML::Node node;
        for (auto &i : _m_loggers)
        {
            //std::cout << i.second->getName() << std::endl;
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

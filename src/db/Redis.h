/*************************************************************************
	> File Name: Redis.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年1月19日 星期四 10时38分01秒
 ************************************************************************/

#ifndef __REDIS_H_
#define __REDIS_H_

#include <string>
#include <memory>
#include <cstdlib>
#include <sys/time.h>

#include <hiredis-vip/hiredis.h>
#include <hiredis-vip/hircluster.h>
#include <hiredis-vip/adapters/libevent.h>

#include "../Thread.h"
#include "../Scheduler.h"
#include "../Singleton.h"
#include "LibevThread.h"

namespace Routn{

    using ReplyPtr = std::shared_ptr<redisReply>;

    class IRedis{
    public:
        enum Type{
            REDIS = 1,
            REDIS_CLUSTER = 2,
            EV_REDIS = 3,
            EV_REDIS_CLUSTER = 4
        };

        using ptr = std::shared_ptr<IRedis>;
        IRedis() : _logEnable(true) {}
        virtual ~IRedis() {}

        virtual ReplyPtr cmd(const char* fmt, ...) = 0;
        virtual ReplyPtr cmd(const char* fmt, va_list ap) = 0;
        virtual ReplyPtr cmd(const std::vector<std::string>& argv) = 0;

        const std::string& getName() const { return _name;}
        void setName(const std::string& v) { _name = v;}

        const std::string& getPasswd() const { return _passwd;}
        void setPasswd(const std::string& v) { _passwd = v;}

        Type getType() const { return _type;}
    protected:
        std::string _name;
        std::string _passwd;
        Type _type;
        bool _logEnable;
    };

    class ISyncRedis : public IRedis{
    public:
        using ptr = std::shared_ptr<ISyncRedis>;
        virtual ~ISyncRedis() {}

        virtual bool reconnect() = 0;
        virtual bool connect(const std::string& ip, int port, uint64_t ms = 0) = 0;
        virtual bool connect() = 0;
        virtual bool setTimeout(uint64_t ms) = 0;

        virtual int appendCmd(const char* fmt, ...) = 0;
        virtual int appendCmd(const char* fmt, va_list ap) = 0;
        virtual int appendCmd(const std::vector<std::string>& argv) = 0;

        virtual ReplyPtr getReply() = 0;

        uint64_t getLastActiveTime() const { return _lastActiveTime;}
        void setLastActiveTime(uint64_t v) { _lastActiveTime = v;}
    protected:
        uint64_t _lastActiveTime;
    };

    class Redis : public ISyncRedis{
    public:
        using ptr = std::shared_ptr<Redis>;
        Redis();
        Redis(const std::map<std::string, std::string>& conf);

        virtual bool reconnect();
        virtual bool connect(const std::string& ip, int port, uint64_t ms = 0);
        virtual bool connect();
        virtual bool setTimeout(uint64_t ms);

        virtual ReplyPtr cmd(const char* fmt, ...);
        virtual ReplyPtr cmd(const char* fmt, va_list ap);
        virtual ReplyPtr cmd(const std::vector<std::string>& argv);

        virtual int appendCmd(const char* cmd, ...);
        virtual int appendCmd(const char* fmt, va_list ap);
        virtual int appendCmd(const std::vector<std::string>& argv);

        virtual ReplyPtr getReply();
    
    private:
        std::string _host;
        uint32_t _port;
        uint32_t _connectMs;
        struct timeval _cmdTimout;
        std::shared_ptr<redisContext> _context;
    };

    class RedisCluster : public ISyncRedis{
    public:
        using ptr = std::shared_ptr<RedisCluster>;
        RedisCluster();
        RedisCluster(const std::map<std::string, std::string>& conf);
            
        virtual bool reconnect();
        virtual bool connect(const std::string& ip, int port, uint64_t ms = 0);
        virtual bool connect();
        virtual bool setTimeout(uint64_t ms);

        virtual ReplyPtr cmd(const char* fmt, ...);
        virtual ReplyPtr cmd(const char* fmt, va_list ap);
        virtual ReplyPtr cmd(const std::vector<std::string>& argv);

        virtual int appendCmd(const char* fmt, ...);
        virtual int appendCmd(const char* fmt, va_list ap);
        virtual int appendCmd(const std::vector<std::string>& argv);

        virtual ReplyPtr getReply();
    private:
        std::string _host;
        uint32_t _port;
        uint32_t _connectMs;
        struct timeval _cmdTimeout;
        std::shared_ptr<redisClusterContext> _context;
    };


    class EvRedis : public IRedis{
    public:
        using ptr = std::shared_ptr<EvRedis>;
        enum STATUS{
            UNCONNECTED = 0,
            CONNECTING = 1,
            CONNECTED = 2
        };
        enum RESULT{
            OK = 0,
            TIME_OUT = 1,
            CONNECT_ERR = 2,
            CMD_ERR = 3,
            REPLY_NULL = 4,
            REPLY_ERR = 5,
            INIT_ERR = 6
        };

        EvRedis(EvThread* thr, const std::map<std::string, std::string>& conf);
        ~EvRedis();

        virtual ReplyPtr cmd(const char* fmt, ...);
        virtual ReplyPtr cmd(const char* fmt, va_list ap);
        virtual ReplyPtr cmd(const std::vector<std::string>& argv);
    
        bool init();
        int getCtxCount() const { return _ctxCount;}
    private:
        static void OnAuthCb(redisAsyncContext* c, void* rp, void* priv);
    private:
        struct ECtx{
            std::string cmd;
            Schedular* scheduler;
            Fiber::ptr fiber;
            ReplyPtr rpy;
        };

        struct Ctx{
            using ptr = std::shared_ptr<Ctx>;

            event* ev;
            bool timeout;
            EvRedis* rds;
            std::string cmd;
            ECtx* ectx;
            EvThread* thread;

            Ctx(EvRedis* rds);
            ~Ctx();
            bool init();
            void cancelEvent();
            static void EventCb(int fd, short event, void* d);
        };
    private:
        virtual void pcmd(ECtx* ctx);
        bool pinit();
        void delayDelete(redisAsyncContext* c);
    private:
        static void ConnectCb(const redisAsyncContext* c, int status);
        static void DisconnectCb(const redisAsyncContext* c, int status);
        static void CmdCb(redisAsyncContext *c, void *r, void *privdata);
        static void TimeCb(int fd, short event, void* d);
    private:
        EvThread* _thread;
        std::shared_ptr<redisAsyncContext> _context;
        std::string _host;
        uint16_t _port;
        STATUS _status;
        int _ctxCount;

        struct timeval _cmdTimeout;
        std::string _err;
        struct event* _event;
    };

    class EvRedisCluster : public IRedis {
    public:
        using ptr = std::shared_ptr<EvRedisCluster>;
        enum STATUS {
            UNCONNECTED = 0,
            CONNECTING = 1,
            CONNECTED = 2
        };
        enum RESULT {
            OK = 0,
            TIME_OUT = 1,
            CONNECT_ERR = 2,
            CMD_ERR = 3,
            REPLY_NULL = 4,
            REPLY_ERR = 5,
            INIT_ERR = 6
        };
        EvRedisCluster(EvThread* thr, const std::map<std::string, std::string>& conf);
        ~EvRedisCluster();

        virtual ReplyPtr cmd(const char* fmt, ...);
        virtual ReplyPtr cmd(const char* fmt, va_list ap);
        virtual ReplyPtr cmd(const std::vector<std::string>& argv);

        int getCtxCount() const { return _ctxCount;}
        bool init();
    private:
        struct ECtx{
            std::string cmd;
            Schedular* scheduler;
            Fiber::ptr fiber;
            ReplyPtr rpy;
        };
        struct Ctx{
            using ptr = std::shared_ptr<Ctx>;
            event* ev;
            bool timeout;
            EvRedisCluster* rds;
            ECtx* ectx;
            std::string cmd;
            EvThread* thread;

            void cancelEvent();
            Ctx(EvRedisCluster* rds);
            ~Ctx();
            bool init();
            static void EventCb(int fd, short event, void* d);
        };
    private:
        virtual void pcmd(ECtx* ctx);
        bool pinit();
        void delayDelete(redisAsyncContext* c);
        static void OnAuthCb(redisClusterAsyncContext* c, void* rp, void* priv);
    private:
        static void ConnectCb(const redisAsyncContext* c, int status);
        static void DisconnectCb(const redisAsyncContext* c, int status);
        static void CmdCb(redisClusterAsyncContext*c, void *r, void *privdata);
        static void TimeCb(int fd, short event, void* d);
    private:
        EvThread* _thread;
        std::shared_ptr<redisClusterAsyncContext> _context;
        std::string _host;
        STATUS _status;
        int _ctxCount;

        struct timeval _cmdTimeout;
        std::string _err;
        struct event* _event;
    };

    class RedisManager{
    public:
        RedisManager();
        IRedis::ptr get(const std::string& name);

        std::ostream& dump(std::ostream& os);
    private:
        void freeRedis(IRedis* r);
        void init();
    private:
        RW_Mutex _mutex;
        std::map<std::string, std::list<IRedis*>> _datas;
        std::map<std::string, std::map<std::string, std::string> > _config;
    };

    using RedisMgr = Singleton<RedisManager>;

    class RedisUtil{
    public:
        static ReplyPtr Cmd(const std::string& name, const char* fmt, ...);
        static ReplyPtr Cmd(const std::string& name, const char* fmt, va_list ap); 
        static ReplyPtr Cmd(const std::string& name, const std::vector<std::string>& args); 

        static ReplyPtr TryCmd(const std::string& name, uint32_t count, const char* fmt, ...);
        static ReplyPtr TryCmd(const std::string& name, uint32_t count, const std::vector<std::string>& args);    
    };
}

#endif
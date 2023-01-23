/*************************************************************************
	> File Name: Redis.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年1月19日 星期四 10时38分01秒
 ************************************************************************/

#include "Redis.h"
#include "../Routn.h"

namespace Routn{
    /**
     * @brief STATIC_ZONE
     * 
     */
    static Logger::ptr g_logger = ROUTN_LOG_NAME("system");
    static ConfigVar<std::map<std::string, std::map<std::string, std::string> > >::ptr 
        g_redis = Config::Lookup("redis.config", std::map<std::string, std::map<std::string, std::string>>(), "redis config");
    static std::string get_value(const std::map<std::string, std::string>& m
                                 , const std::string& key
                                 , const std::string& def = ""){
        auto it = m.find(key);
        return it == m.end() ? def : it->second;
    }

    redisReply* RedisReplyClone(redisReply* r){
        redisReply* c = (redisReply*)calloc(1, sizeof(*c));
        c->type = r->type;

        switch (r->type)
        {
        case REDIS_REPLY_INTEGER:
            c->integer = r->integer;
            break;
        case REDIS_REPLY_ARRAY:
            if(r->element != NULL && r->element > 0){
                c->element = (redisReply**)calloc(r->elements, sizeof(r));
                c->elements = r->elements;
                for(size_t i = 0; i < r->elements; ++i){
                    c->element[i] = RedisReplyClone(r->element[i]);
                }
            }
            break;
        case REDIS_REPLY_ERROR:
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_STRING:
            if(r->str == NULL){
                c->str = NULL;
            }else{
                c->str = (char*)malloc(r->len + 1);
                memcpy(c->str, r->str, r->len);
                c->str[r->len] = '\0';
            }
            c->len = r->len;
            break;
        }
        return c;
    }

    Redis::Redis(){
        _type = IRedis::REDIS;
    }

    Redis::Redis(const std::map<std::string, std::string>& conf){
        _type = IRedis::REDIS;
        auto tmp = get_value(conf, "host");
        auto pos = tmp.find(":");
        _host = tmp.substr(0, pos);
        _port = atoi(tmp.substr(pos + 1).c_str());
        _passwd = get_value(conf, "passwd");
        _logEnable = atoi(get_value(conf, "log_enable", "1").c_str());

        tmp = get_value(conf, "timeout_com");
        if(tmp.empty()){
            tmp = get_value(conf, "timeout");
        }
        uint64_t v = atoi(tmp.c_str());

        _cmdTimout.tv_sec = v / 1000;
        _cmdTimout.tv_usec = v % 1000 * 1000;
    }

    bool Redis::reconnect(){
        return redisReconnect(_context.get());
    }

    bool Redis::connect(const std::string& ip, int port, uint64_t ms = 0){
        _host = ip;
        _port = port;
        _connectMs = ms;
        if(_context){
            return true;
        }
        timeval tv = {(int)ms / 1000, (int)ms % 1000 * 1000};
        auto c = redisConnectWithTimeout(ip.c_str(), port, tv);
        if(c){
            if(_cmdTimout.tv_sec || _cmdTimout.tv_usec){
                setTimeout(_cmdTimout.tv_sec * 1000 + _cmdTimout.tv_usec / 1000);
            }
            _context.reset(c, redisFree);

            if(!_passwd.empty()){
                auto r = (redisReply*)redisCommand(c, "auth %s", _passwd.c_str());
                if(!r){
                    ROUTN_LOG_ERROR(g_logger) << "auth error:("
                        << _host << ":" << _port << ", " << _name << ")";
                    return false;
                }
                if(r->type != REDIS_REPLY_STATUS){
                    ROUTN_LOG_ERROR(g_logger) << "auth reply type error:" << r->type << "("
                    << _host << ":" << _port << ", " << _name << ")";
                    return false;
                }
                if(!r->str){
                    ROUTN_LOG_ERROR(g_logger) << "auth reply str error: NULL("
                        << _host << ":" << _port << ", " << _name << ")";
                    return false;
                }
                if(strcmp(r->str, "OK") == 0){
                    return true;
                }else{
                    ROUTN_LOG_ERROR(g_logger) << "auth error:("
                        << _host << ":" << _port << ", " << _name << ")";
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    bool Redis::connect(){
        return connect(_host, _port, 50);
    }

    bool Redis::setTimeout(uint64_t ms){
        _cmdTimout.tv_sec = ms / 1000;
        _cmdTimout.tv_usec = ms % 1000 * 1000;
        redisSetTimeout(_context.get(), _cmdTimout);
        return true;
    }


    ReplyPtr Redis::cmd(const char* fmt, ...){
        va_list ap;
        va_start(ap, fmt);
        ReplyPtr rt = cmd(fmt, ap);
        va_end(ap);
        return rt;
    }

    ReplyPtr Redis::cmd(const char* fmt, va_list ap){
        auto r = (redisReply*)redisvCommand(_context.get(), fmt, ap);
        if(!r){
            if(_logEnable){
                ROUTN_LOG_ERROR(g_logger) << "redisCommand error : (" << fmt << ")("
                    << _host << ":" << _port << ", " << _name << ")";
                return nullptr;
            }
        }
        ReplyPtr rt(r, freeReplyObject);
        if(r->type != REDIS_REPLY_ERROR){
            return rt;
        }
        if(_logEnable){
            ROUTN_LOG_ERROR(g_logger) << "redisCommand error : (" << fmt << ")("
            << _host << ":" << _port << ", " << _name << ")";
        }
        return nullptr;
    }

    ReplyPtr Redis::cmd(const std::vector<std::string>& argv){
        std::vector<const char*> v;
        std::vector<size_t> l;
        for(auto& i : argv){
            v.push_back(i.c_str());
            l.push_back(i.size());
        }
        auto r = (redisReply*)redisCommandArgv(_context.get(), argv.size(), &v[0], &l[0]);
        if(!r){
            if(_logEnable){
                ROUTN_LOG_ERROR(g_logger) << "redisCommandArgv error: " << _host << ":" << _port << ", " << _name << ")";
            }
            return nullptr;
        }
        ReplyPtr rt(r, freeReplyObject);
        if(r->type != REDIS_REPLY_ERROR){
            return rt;
        }
        if(_logEnable){
            ROUTN_LOG_ERROR(g_logger) << "redisCommandArgv error: " << _host << ":" << _port << ", " << _name << ")";
        }
        return nullptr;
    }


    int Redis::appendCmd(const char* cmd, ...){
        va_list ap;
        va_start(ap, cmd);
        int rt = appendCmd(cmd, ap);
        va_end(ap);
        return rt;
    }

    int Redis::appendCmd(const char* fmt, va_list ap){
        return redisvAppendCommand(_context.get(), fmt, ap);
    }

    int Redis::appendCmd(const std::vector<std::string>& argv){
        std::vector<const char*> v;
        std::vector<size_t> l;
        for(auto& i : argv){
            v.push_back(i.c_str());
            l.push_back(i.size());
        }
        return redisAppendCommandArgv(_context.get(), argv.size(), &v[0], &l[0]);
    }


    ReplyPtr Redis::getReply(){
        redisReply* r = nullptr;
        if(redisGetReply(_context.get(), (void**)&r) == REDIS_OK){
            ReplyPtr rt(r, freeReplyObject);
            return rt;
        }
        if(_logEnable){
            ROUTN_LOG_ERROR(g_logger) << "redisGetReply error : (" << _host << ":" << _port << ", " << _name << ")";
        }
        return nullptr;
    }


    RedisCluster::RedisCluster(){
        _type = IRedis::REDIS_CLUSTER;
    }

    RedisCluster::RedisCluster(const std::map<std::string, std::string>& conf){
        _type = IRedis::REDIS_CLUSTER;
        _host = get_value(conf, "host");
        _passwd = get_value(conf, "passwd");
        _logEnable = atoi(get_value(conf, "log_enable", "1").c_str());
        auto tmp = get_value(conf, "timeout_com");
        if(tmp.empty()){
            tmp = get_value(conf, "timeout");
        }
        uint64_t v = atoi(tmp.c_str());
        _cmdTimeout.tv_sec = v / 1000;
        _cmdTimeout.tv_usec = v % 1000 * 1000;
    }


    bool RedisCluster::reconnect(){
        return true;
    }

    bool RedisCluster::connect(const std::string& ip, int port, uint64_t ms = 0){
        _host = ip;
        _port = port;
        _connectMs = ms;
        if(_context){
            return true;
        }
        timeval tv = {(int)ms / 1000, (int)ms % 1000 * 1000};
        auto c = redisClusterConnectWithTimeout(ip.c_str(), tv, 0);
        if(c){
            _context.reset(c, redisClusterFree);
            if(!_passwd.empty()){
                auto r = (redisReply*)redisClusterCommand(c, "auth %s", _passwd.c_str());
                if(!r){
                    ROUTN_LOG_ERROR(g_logger) << "auth error: ("
                        << _host << ":" << _port << ", " << _name << ")";
                    return false;
                }
                if(r->type != REDIS_REPLY_STATUS){
                    ROUTN_LOG_ERROR(g_logger) << "auth reply type error: ("
                        << _host << ":" << _port << ", " << _name << ")";
                    return false;
                }
                if(!r->str){
                    ROUTN_LOG_ERROR(g_logger) << "auth reply str error: ("
                        << _host << ":" << _port << ", " << _name << ")";
                    return false;
                }
                if(strcmp(r->str, "OK") == 0){
                    return true;
                }else{
                    ROUTN_LOG_ERROR(g_logger) << "auth error: ("
                        << _host << ":" << _port << ", " << _name << ")";
                    return false;
                }
            }
            return true;
        }
        return false;
    }
    
    bool RedisCluster::connect(){
        return connect(_host, _port, 50);
    }
    
    bool RedisCluster::setTimeout(uint64_t ms){
        return true;
    }


    ReplyPtr RedisCluster::cmd(const char* fmt, ...){
        va_list ap;
        va_start(ap, fmt);
        ReplyPtr rt = cmd(fmt, ap);
        va_end(ap);
        return rt;
    }

    ReplyPtr RedisCluster::cmd(const char* fmt, va_list ap){
        auto r = (redisReply*)redisClusterCommand(_context.get(), fmt, ap);
        if(!r){
            if(_logEnable){
                ROUTN_LOG_ERROR(g_logger) << "redisCommand error: (" << fmt << ")(" << _host << ":" << _port << ", " << _name << ")";
            }
            return nullptr;
        }
        ReplyPtr rt(r, freeReplyObject);
        if(r->type != REDIS_REPLY_ERROR){
            return rt;
        }
        if(_logEnable){
            ROUTN_LOG_ERROR(g_logger) << "redisCommand error: (" << fmt << ")(" << _host << ":" << _port << ", " << _name << ")";
        }
        return nullptr;
    }

    ReplyPtr RedisCluster::cmd(const std::vector<std::string>& argv){
        std::vector<const char*> v;
        std::vector<size_t> l;
        for(auto& i : argv){
            v.push_back(i.c_str());
            l.push_back(i.size());
        }

        auto r = (redisReply*)redisClusterCommandArgv(_context.get(), argv.size(), &v[0], &l[0]);
        if(!r){
            if(_logEnable){
                ROUTN_LOG_ERROR(g_logger) << "redisCommandArgv error: (" << _host << ":" << _port << ", " << _name << ")";
            }
            return nullptr;
        }
        ReplyPtr rt(r, freeReplyObject);
        if(r->type != REDIS_REPLY_ERROR){
            return rt;
        }
        if(_logEnable){
            ROUTN_LOG_ERROR(g_logger) << "redisCommandArgv error: (" << _host << ":" << _port << ", " << _name << ")";
        }
        return nullptr;
    }


    int RedisCluster::appendCmd(const char* fmt, ...){
        va_list ap;
        va_start(ap, fmt);
        int rt = appendCmd(fmt, ap);
        va_end(ap);
        return rt;
    }

    int RedisCluster::appendCmd(const char* fmt, va_list ap){
        return redisClustervAppendCommand(_context.get(), fmt, ap);
    }

    int RedisCluster::appendCmd(const std::vector<std::string>& argv){
        std::vector<const char*> v;
        std::vector<size_t> l;
        for(auto& i : argv){
            v.push_back(i.c_str());
            l.push_back(i.size());
        }

        return redisClusterAppendCommandArgv(_context.get(), argv.size(), &v[0], &l[0]);
    }


    ReplyPtr RedisCluster::getReply(){
        redisReply* r = nullptr;
        if(redisClusterGetReply(_context.get(), (void**)&r) == REDIS_OK){
            ReplyPtr rt(r, freeReplyObject);
            return rt;
        }
        if(_logEnable){
            ROUTN_LOG_ERROR(g_logger) << "redisGetReply error: (" << _host << ":" << _port << ", " << _name << ")";
        }
        return nullptr;
    }


    EvRedis::EvRedis(EvThread* thr, const std::map<std::string, std::string>& conf)
        : _thread(thr)
        , _status(UNCONNECTED)
        , _event(nullptr) {
        _type = IRedis::EV_REDIS;
        auto tmp = get_value(conf, "host");
        auto pos = tmp.find(":");
        _host = tmp.substr(0, pos);
        _port = atoi(tmp.substr(pos + 1).c_str());
        _passwd = get_value(conf, "passwd");
        _ctxCount = 0;
        _logEnable = atoi(get_value(conf, "log_enable", "1").c_str());

        tmp = get_value(conf, "timeout_com");
        if(tmp.empty()){
            tmp = get_value(conf, "timeout");
        }
        uint64_t v = atoi(tmp.c_str());
        _cmdTimeout.tv_sec = v / 1000;
        _cmdTimeout.tv_usec = v % 1000 * 1000;
    }

    EvRedis::~EvRedis(){
        if(_event){
            evtimer_del(_event);
            event_free(_event);
        }
    }


    ReplyPtr EvRedis::cmd(const char* fmt, ...){
        va_list ap;
        va_start(ap, fmt);
        auto r = cmd(fmt, ap);
        va_end(ap);
        return r;
    }

    ReplyPtr EvRedis::cmd(const char* fmt, va_list ap){
        char* buf = nullptr;
        int len = redisvFormatCommand(&buf, fmt, ap);
        if(len == -1){
            ROUTN_LOG_ERROR(g_logger) << "redis fmt error: " << fmt;
            return nullptr;
        }
        ECtx ectx;
        ectx.cmd.append(buf, len);
        free(buf);
        ectx.scheduler = Schedular::GetThis();
        ectx.fiber = Fiber::GetThis();

        _thread->dispatch(std::bind(&EvRedis::pcmd, this, &ectx));
        Fiber::YieldToHold();
        return ectx.rpy;
    }

    ReplyPtr EvRedis::cmd(const std::vector<std::string>& argv){
        ECtx ctx;
        do{
            std::vector<const char*> args;
            std::vector<size_t> args_len;
            for(auto& i : argv) {
                args.push_back(i.c_str());
                args_len.push_back(i.size());
            }
            char* buf = nullptr;
            int len = redisFormatCommandArgv(&buf, argv.size(), &(args[0]), &(args_len[0]));
            if(len == -1 || !buf) {
                ROUTN_LOG_ERROR(g_logger) << "redis fmt error";
                return nullptr;
            }
            ctx.cmd.append(buf, len);
            free(buf);
        }while(false);

        ctx.scheduler = Schedular::GetThis();
        ctx.fiber = Fiber::GetThis();

        _thread->dispatch(std::bind(&EvRedis::pcmd, this, &ctx));
        Fiber::YieldToHold();
        return ctx.rpy;
    }


    bool EvRedis::init(){
        if(_thread == EvThread::GetThis()){
            return pinit();
        }else{
            _thread->dispatch(std::bind(&EvRedis::pinit, this));
        }
        return true;
    }

    EvRedis::Ctx::Ctx(EvRedis* rds)
        : ev(nullptr)
        , timeout(false)
        , rds(rds) 
        , thread(nullptr) {
        Atomic::addFetch(rds->_ctxCount, 1);
    }

    EvRedis::Ctx::~Ctx(){
        ROUTN_ASSERT(thread == EvThread::GetThis());
        Atomic::subFetch(rds->_ctxCount, 1);
        if(ev){
            evtimer_del(ev);
            event_free(ev);
            ev = nullptr;
        }
    }

    bool EvRedis::Ctx::init(){
        ev = evtimer_new(rds->_thread->getBase(), EventCb, this);
        evtimer_add(ev, &rds->_cmdTimeout);
        return true;
    }

    void EvRedis::Ctx::cancelEvent(){
        /// TODO
    }

    void EvRedis::Ctx::EventCb(int fd, short event, void* d){
        Ctx*ctx = static_cast<Ctx*>(d);
        ctx->timeout = 1;
        if(ctx->rds->_logEnable){
            ROUTN_LOG_INFO(g_logger) << "redis cmd" << ctx->cmd << " reach timeout "
                << (ctx->rds->_cmdTimeout.tv_sec * 1000 + ctx->rds->_cmdTimeout.tv_usec / 1000) << "ms";
        }
        if(ctx->ectx->fiber){
            ctx->ectx->scheduler->schedule(&ctx->ectx->fiber);
        }
        ctx->cancelEvent();
    }


    void EvRedis::pcmd(ECtx* ctx){
        if(_status == UNCONNECTED){
            ROUTN_LOG_INFO(g_logger) << "redis (" << _host << ":" << _port << ") unconnected"
                << ctx->cmd;
            init();
            if(ctx->fiber){
                ctx->scheduler->schedule(&ctx->fiber);
            }
            return;
        }
        Ctx* cctx(new Ctx(this));
        cctx->thread = _thread;
        cctx->init();
        cctx->ectx = ctx;
        cctx->cmd = ctx->cmd;

        if(!cctx->cmd.empty()){
            redisAsyncFormattedCommand(_context.get(), CmdCb, cctx, cctx->cmd.c_str(), cctx->cmd.size());
        }
    }

    bool EvRedis::pinit(){
        if(_status != UNCONNECTED){
            return true;
        }
        auto ctx = redisAsyncConnect(_host.c_str(), _port);
        if(!ctx){
            ROUTN_LOG_ERROR(g_logger) << "redisAsyncConnect: (" << _host << ":" << _port << ", " << _name << ") null";
            return false;
        }
        if(ctx->err){
            ROUTN_LOG_ERROR(g_logger) << "Error: (" << ctx->err << ")" << ctx->errstr;
            return false;
        }
        ctx->data = this;
        redisLibeventAttach(ctx, _thread->getBase());
        redisAsyncSetConnectCallback(ctx, ConnectCb);
        redisAsyncSetDisconnectCallback(ctx, DisconnectCb);
        _status = CONNECTING;
        _context.reset(ctx, nop<redisAsyncContext>);
        if(_event == nullptr){
            _event = event_new(_thread->getBase(), -1, EV_TIMEOUT | EV_PERSIST, TimeCb, this);
            struct timeval tv = {120, 0};
            evtimer_add(_event, &tv);
        }
        TimeCb(0, 0, this);
        return true;
    }

    void EvRedis::delayDelete(redisAsyncContext* c){
        //TODO
    }   

    void EvRedis::ConnectCb(const redisAsyncContext* c, int status){
        EvRedis* ar = static_cast<EvRedis*>(c->data);
        if(!status){
            ROUTN_LOG_INFO(g_logger) << "EvRedis::ConnectCb "
                << c->c.tcp.host << ":" << c->c.tcp.port
                << " success";
            ar->_status = CONNECTED;
            if(!ar->_passwd.empty()){
                int rt = redisAsyncCommand(ar->_context.get(), EvRedis::OnAuthCb, ar, "auth %s", ar->_passwd.c_str());
                if(rt){
                    ROUTN_LOG_ERROR(g_logger) << "EvRedis Auth fail:" << rt;
                }
            }
        }else{
            ROUTN_LOG_ERROR(g_logger) << "EvRedis::ConnectCb "
                << c->c.tcp.host << ":" << c->c.tcp.port
                << " fail, error: " << c->errstr;
            ar->_status = UNCONNECTED;
        }
    }

    void EvRedis::CmdCb(redisAsyncContext *c, void *r, void *privdata){
        Ctx* ctx = static_cast<Ctx*>(privdata);
        if(!ctx){
            return ;
        }
        if(ctx->timeout){
            delete ctx;
            return ;
        }
        auto _log = ctx->rds->_logEnable;
        redisReply* reply = (redisReply*)r;
        if(c->err){
            if(_log){
                ROUTN_LOG_ERROR(g_logger) << "redis cmd: " << ctx->cmd
                    << " (" << c->err << ")" << c->errstr;
            }
            if(ctx->ectx->fiber){
                ctx->ectx->scheduler->schedule(&ctx->ectx->fiber);
            }
        }else if(!reply){
            if(_log){
                ROUTN_LOG_ERROR(g_logger) << "redis cmd: " << ctx->cmd
                    << " (" << c->err << ") null, " << c->errstr;
            }
            if(ctx->ectx->fiber){
                ctx->ectx->scheduler->schedule(&ctx->ectx->fiber);
            }
        }else if(reply->type == REDIS_REPLY_ERROR){
            if(_log){
                ROUTN_LOG_ERROR(g_logger) << "redis cmd error";
            }
            if(ctx->ectx->fiber){
                ctx->ectx->scheduler->schedule(&ctx->ectx->fiber);
            }
        }else{
            if(ctx->ectx->fiber){
                ctx->ectx->rpy.reset(RedisReplyClone(reply), freeReplyObject);
                ctx->ectx->scheduler->schedule(&ctx->ectx->fiber);
            }
        }
        ctx->cancelEvent();
        delete ctx;
    }

    void EvRedis::DisconnectCb(const redisAsyncContext* c, int status){
        ROUTN_LOG_INFO(g_logger) << "EvRedis::DisconnectCb "
            << c->c.tcp.host << ":" << c->c.tcp.port
            << " status: " << status;
        EvRedis* ar = static_cast<EvRedis*>(c->data);
        ar->_status = UNCONNECTED;
    }

    void EvRedis::TimeCb(int fd, short event, void* d){
        EvRedis* ar = static_cast<EvRedis*>(d);
        redisAsyncCommand(ar->_context.get(), CmdCb, nullptr, "ping");
    }


    EvRedisCluster::EvRedisCluster(EvThread* thr, const std::map<std::string, std::string>& conf){

    }

    EvRedisCluster::~EvRedisCluster(){

    }

    ReplyPtr EvRedisCluster::cmd(const char* fmt, ...){

    }

    ReplyPtr EvRedisCluster::cmd(const char* fmt, va_list ap){

    }

    ReplyPtr EvRedisCluster::cmd(const std::vector<std::string>& argv){

    }


    bool EvRedisCluster::init(){

    }


    void EvRedisCluster::Ctx::cancelEvent(){

    }
    
    EvRedisCluster::Ctx::Ctx(EvRedisCluster* rds){

    }

    EvRedisCluster::Ctx::~Ctx(){

    }
    bool EvRedisCluster::Ctx::init(){

    }

    void EvRedisCluster::Ctx::EventCb(int fd, short event, void* d){

    }

    void EvRedisCluster::pcmd(ECtx* ctx){

    }

    bool EvRedisCluster::pinit(){

    }

    void EvRedisCluster::delayDelete(redisAsyncContext* c){

    }

    void EvRedisCluster::TimeCb(int fd, short event, void* d){

    }

    void EvRedisCluster::ConnectCb(const redisAsyncContext* c, int status){

    }

    void EvRedisCluster::DisconnectCb(const redisAsyncContext* c, int status){

    }

    void EvRedisCluster::CmdCb(redisClusterAsyncContext*c, void *r, void *privdata){

    }

    void EvRedisCluster::OnAuthCb(redisClusterAsyncContext* c, void* rp, void* priv){

    }



    RedisManager::RedisManager(){

    }

    IRedis::ptr RedisManager::get(const std::string& name){

    }

    std::ostream& RedisManager::dump(std::ostream& os){

    }

    void RedisManager::freeRedis(IRedis* r){

    }

    void RedisManager::init(){

    }




    ReplyPtr RedisUtil::Cmd(const std::string& name, const char* fmt, ...){

    }

    ReplyPtr RedisUtil::Cmd(const std::string& name, const char* fmt, va_list ap){

    }

    ReplyPtr RedisUtil::Cmd(const std::string& name, const std::vector<std::string>& args){

    }


    ReplyPtr RedisUtil::TryCmd(const std::string& name, uint32_t count, const char* fmt, ...){

    }

    ReplyPtr RedisUtil::TryCmd(const std::string& name, uint32_t count, const std::vector<std::string>& args){

    }


}
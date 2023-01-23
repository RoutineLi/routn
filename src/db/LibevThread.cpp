#include "LibevThread.h"       
#include "../Routn.h"
#include <iomanip> 

namespace Routn{    
/**
 * @brief STATIC_ZONE
 * 
 */

    static Routn::ConfigVar<std::map<std::string, std::map<std::string, std::string>>>::ptr 
        g_thread_info_set = Config::Lookup("ev_thread", std::map<std::string, std::map<std::string, std::string>>()
            , "config for libevent_thread");

    static RW_Mutex s_thread_mutex;
    static std::map<uint64_t, std::string> s_thread_names;

    thread_local EvThread* s_thread = nullptr;

/**
 * @brief Construct a new Ev Thread:: Ev Thread object
 * 
 * @param name 
 * @param base 
 */
    EvThread::EvThread(const std::string& name
                    , struct event_base* base)
        : _read(0)
        , _write(0)
        , _base(NULL)
        , _event(NULL)
        , _thread(NULL)
        , _name(name)
        , _working(false)
        , _start(false)
        , _total(0) {
        int fds[2];
        if(evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1){
            throw std::logic_error("thread init error");
        }

        evutil_make_socket_nonblocking(fds[0]);
        evutil_make_socket_nonblocking(fds[1]);

        _read = fds[0];
        _write = fds[1];

        if(base){
            _base = base;
            setThis();
        }else{
            _base = event_base_new();
        }
        _event = event_new(_base, _read, EV_READ | EV_PERSIST, read_cb, this);
        event_add(_event, NULL);
    }
/**
 * @brief Destroy the Ev Thread:: Ev Thread object
 * 
 */
    EvThread::~EvThread(){
        if(_read){
            close(_read);
        }
        if(_write){
            close(_write);
        }
        stop();
        join();
        if(_thread) {
            delete _thread;
        }
        if(_event){
            event_free(_event);
        }
        if(_base){
            event_base_free(_base);
        }
    }
/**
 * @brief Get this thread
 * 
 * @return EvThread* 
 */
    EvThread* EvThread::GetThis(){
        return s_thread;
    }

/**
 * @brief Get this thread name
 * 
 * @return const std::string& 
 */
    const std::string& EvThread::GetEvThreadName(){
        EvThread* t = GetThis();
        if(t){
            return t->_name;
        }

        uint64_t tid = GetThreadId();
        do{
            RW_Mutex::WriteLock lock(s_thread_mutex);
            s_thread_names[tid] = "libevent_" + std::to_string(tid);
            return s_thread_names[tid];
        }while(false);
        
    }
/**
 * @brief get all thread name
 * 
 * @param names 
 */
    void EvThread::GetAllEvThreadName(std::map<uint64_t, std::string>& names){
        RW_Mutex::ReadLock lock(s_thread_mutex);
        for(auto it = s_thread_names.begin(); it != s_thread_names.end(); ++it){
            names.emplace(*it);
        }
    }

/**
 * @brief set this thread
 * 
 */
    void EvThread::setThis(){
        _name = _name + "_" + std::to_string(GetThreadId());
        s_thread = this;

        RW_Mutex::WriteLock lock(s_thread_mutex);
        s_thread_names[GetThreadId()] = _name;
    }

/**
 * @brief cancel this thread
 * 
 */
    void EvThread::unsetThis(){
        s_thread = nullptr;
        RW_Mutex::WriteLock lock(s_thread_mutex);
        s_thread_names.erase(GetThreadId());
    }

/**
 * @brief start thread
 * 
 */
    void EvThread::start(){
        if(_thread){
            throw std::logic_error("EvThread is running");
        }
        _thread = new std::thread(std::bind(&EvThread::thread_cb, this));
        _start = true;
    }

/**
 * @brief thread run task
 * 
 * @param cb 
 * @return true 
 * @return false 
 */
    bool EvThread::dispatch(callback cb){
        RW_Mutex::WriteLock lock(_mutex);
        _callbacks.emplace_back(cb);

        lock.unlock();
        uint8_t cmd = 1;
        if(send(_write, &cmd, sizeof(cmd), 0) <= 0){
            return false;
        }
        return true;
    }

/**
 * @brief thread run task
 * 
 * @param id 
 * @param cb 
 * @return true 
 * @return false 
 */
    bool EvThread::dispatch(uint32_t id, callback cb){
        return dispatch(cb);
    }

/**
 * @brief thread run tasks as batch-mode
 * 
 * @param cbs 
 * @return true 
 * @return false 
 */
    bool EvThread::batchDispatch(const std::vector<callback>& cbs){
        RW_Mutex::WriteLock lock(_mutex);
        for(auto& i : cbs){
            _callbacks.push_back(i);
        }
        lock.unlock();
        uint8_t cmd = 1;
        if(send(_write, &cmd, sizeof(cmd), 0) <= 0){
            return false;
        }
        return true;
    }

/**
 * @brief thread broadcast
 * 
 * @param cb 
 */
    void EvThread::broadcast(callback cb){
        dispatch(cb);
    }

    void EvThread::join(){
        if(_thread){
            _thread->join();
            delete _thread;
            _thread = NULL;
        }
    }

/**
 * @brief thread stop running
 * 
 */
    void EvThread::stop(){
        RW_Mutex::WriteLock lock(_mutex);
        _callbacks.push_back(nullptr);
        if(_thread){
            uint8_t cmd = 0;
            send(_write, &cmd, sizeof(cmd), 0);
        }
    }

/**
 * @brief serialize thread-status to stream
 * 
 * @param os 
 */
    void EvThread::dump(std::ostream& os){
        RW_Mutex::ReadLock lock(_mutex);
        os << "[thread name = " << _name
           << " working = " << _working
           << " tasks = " << _callbacks.size()
           << " total = " << _total
           << "]" << std::endl;
    }

/**
 * @brief thread_callback
 * 
 */
    void EvThread::thread_cb(){
        setThis();
        pthread_setname_np(pthread_self(), _name.substr(0, 15).c_str());
        if(_initCb){
            _initCb(this);
            _initCb = nullptr;
        }
        event_base_loop(_base, 0);
    }

/**
 * @brief read_callback
 * 
 * @param sock 
 * @param which 
 * @param args 
 */
    void EvThread::read_cb(evutil_socket_t sock, short which, void* args){
        EvThread* thread = static_cast<EvThread*>(args);
        uint8_t cmd[4096];
        if(recv(sock, cmd, sizeof(cmd), 0) > 0){
            std::list<callback> callbacks;
            RW_Mutex::WriteLock lock(thread->_mutex);
            callbacks.swap(thread->_callbacks);
            lock.unlock();
            thread->_working = true;
            for(auto it = callbacks.begin(); it != callbacks.end(); ++it){
                if(*it){
                    try{
                        (*it)();
                    }catch(std::exception& e){
                        ROUTN_LOG_ERROR(g_logger) << "exception: " << e.what();
                    }catch(const char* c){
                        ROUTN_LOG_ERROR(g_logger) << "exception: " << c;
                    }catch(...){
                        ROUTN_LOG_ERROR(g_logger) << "uncatch exception";
                    }
                }else{
                    event_base_loopbreak(thread->_base);
                    thread->_start = false;
                    thread->unsetThis();
                    break;
                }
            }
            Atomic::addFetch(thread->_total, callbacks.size());
            thread->_working = false;
        }
    }


/**
 * @brief Construct a new Ev Thread Pool:: Ev Thread Pool object
 * 
 * @param size 
 * @param name 
 * @param advance 
 */
    EvThreadPool::EvThreadPool(uint32_t size, const std::string& name, bool advance)
        : _size(size)
        , _cur(0)
        , _name(name)
        , _advance(advance)
        , _start(false)
        , _total(0) {
        _threads.resize(_size);
        for(size_t i = 0; i < size; ++i){
            EvThread* t(new EvThread(name + "_" + std::to_string(i)));
            _threads[i] = t;
        }
    }

/**
 * @brief Destroy the Ev Thread Pool:: Ev Thread Pool object
 * 
 */
    EvThreadPool::~EvThreadPool(){
        for(size_t i = 0; i < _size; ++i){
            delete _threads[i];
        }
    }

/**
 * @brief start libevent pool 
 * 
 */
    void EvThreadPool::start(){ 
        for(size_t i = 0; i < _size; ++i){
            _threads[i]->setInitCb(_initCb);
            _threads[i]->start();
            _freeEvThreads.push_back(_threads[i]);
        }
        if(_initCb){
            _initCb = nullptr;
        }
        _start = true;
        check();
    }

    void EvThreadPool::stop(){
        for(size_t i = 0; i < _size; ++i){
            _threads[i]->stop();
        }
        _start = false;
    }

    void EvThreadPool::join(){
        for(size_t i = 0; i < _size; ++i){
            _threads[i]->join();
        }
    }

    //random thread dispatch
    bool EvThreadPool::dispatch(callback cb){
        do{
            Atomic::addFetch(_total, (uint64_t)1);
            RW_Mutex::WriteLock lock(_mutex);
            if(!_advance){
                return _threads[_cur++ % _size]->dispatch(cb);
            }
            _callbacks.push_back(cb);
        }while(false);
        check();
        return true;
    }

    bool EvThreadPool::batchDispatch(const std::vector<callback>& cbs){
        Atomic::addFetch(_total, cbs.size());
        RW_Mutex::WriteLock lock(_mutex);
        if(!_advance){
            for(auto cb : cbs){
                _threads[_cur++ % _size]->dispatch(cb);
            }
            return true;
        }
        for(auto cb : cbs){
            _callbacks.push_back(cb);
        }
        lock.unlock();
        check();
        return true;
    }

    bool EvThreadPool::dispatch(uint32_t id, callback cb){
        Atomic::addFetch(_total, (uint64_t)1);
        return _threads[id % _size]->dispatch(cb);
    }

/**
 * @brief get random thread
 * 
 * @return EvThread* 
 */
    EvThread* EvThreadPool::getRandEvThread(){
        return _threads[_cur++ % _size];
    }

/**
 * @brief dump func
 * 
 * @param os 
 */
    void EvThreadPool::dump(std::ostream& os){
        RW_Mutex::ReadLock lock(_mutex);
        os << "[FoxThreadPool name = " << _name << " thread_count = " << _threads.size()
           << " tasks = " << _callbacks.size() << " total = " << _total
           << " advance = " << _advance
           << "]" << std::endl;
        for(size_t i = 0; i < _threads.size(); ++i){
            os  <<  " ";
            _threads[i]->dump(os);
        } 
    }

/**
 * @brief threadpool broadcast
 * 
 * @param cb 
 */
    void EvThreadPool::broadcast(callback cb){
        for(size_t i = 0; i < _threads.size(); ++i){
            _threads[i]->dispatch(cb);
        }
    }

/**
 * @brief put release thread into free-list
 * 
 * @param t 
 */
    void EvThreadPool::releaseEvThread(EvThread* t){
        do{
            RW_Mutex::WriteLock lock(_mutex);
            _freeEvThreads.push_back(t);
        }while(false);
        check();
    }

/**
 * @brief check threads
 * 
 */
    void EvThreadPool::check(){
        do{
            if(!_start){
                break;
            }
            RW_Mutex::WriteLock lock(_mutex);
            if(_freeEvThreads.empty() || _callbacks.empty()){
                break;
            }

            EvThread::ptr thr(_freeEvThreads.front(), std::bind(&EvThreadPool::releaseEvThread
                            , this, std::placeholders::_1));
            _freeEvThreads.pop_front();

            callback cb = _callbacks.front();
            _callbacks.pop_front();
            lock.unlock();

            if(thr->isStart()){
                thr->dispatch(std::bind(&EvThreadPool::wrapCb, this, thr, cb));
            }else{
                RW_Mutex::WriteLock lock(_mutex);
                _callbacks.push_front(cb);
            }
        }while(true);
    }

/**
 * @brief wrap callback and do callback
 * 
 * @param cb 
 */
    void EvThreadPool::wrapCb(std::shared_ptr<EvThread>, callback cb){
        cb();
    }


/**
 * @brief dispatch callback
 * 
 * @param name 
 * @param cb 
 */
    void EvThreadManager::dispatch(const std::string& name, callback cb){
        IEvThread::ptr ti = get(name);
        ROUTN_ASSERT(ti);
        ti->dispatch(cb);
    }

    void EvThreadManager::dispatch(const std::string& name, uint32_t id, callback cb){
        IEvThread::ptr ti = get(name);
        ROUTN_ASSERT(ti);
        ti->dispatch(id, cb);
    }

    void EvThreadManager::batchDispatch(const std::string& name, const std::vector<callback>& cbs){
        IEvThread::ptr ti = get(name);
        ROUTN_ASSERT(ti);
        ti->batchDispatch(cbs);
    }

    void EvThreadManager::broadcast(const std::string& name, callback cb){
        IEvThread::ptr ti = get(name);
        ROUTN_ASSERT(ti);
        ti->broadcast(cb);
    }


    void EvThreadManager::dumpEvThreadStatus(std::ostream& os){
        os << "EvThreadManager: " << std::endl;
        for(auto it = _threads.begin()
            ; it != _threads.end(); ++it){
            it->second->dump(os);
        }
        os << "All EvThreads: " << std::endl;
        std::map<uint64_t, std::string> names;
        EvThread::GetAllEvThreadName(names);
        for(auto it = names.begin(); it != names.end(); ++it){
            os << std::setw(30) << it->first
               << ": " << it->second << std::endl;
        }
    }

/**
 * @brief init ThreadMgr
 * 
 */
    void EvThreadManager::init(){
        auto m = g_thread_info_set->getValue();
        for(auto i : m){
            auto num = GetParamValue(i.second, "num", 0);
            auto name = i.first;
            auto advance = GetParamValue(i.second, "advance", 0);
            if(num <= 0){
                ROUTN_LOG_ERROR(g_logger) << "thread pool: " << name
                    << " num: " << num
                    << " advance: " << advance
                    << " is invalid";
                continue;
            }
            if(num == 1){
                _threads[i.first] = EvThread::ptr(new EvThread(i.first));
                ROUTN_LOG_INFO(g_logger) << "init thread: " << i.first;
            }else{
                _threads[i.first] = EvThreadPool::ptr(new EvThreadPool(num, name, advance));
                ROUTN_LOG_INFO(g_logger) << "init thread pool: " << name
                    << " num: " << num
                    << " advance: " << advance;
            }
        }
    }

/**
 * @brief EvThreadMgr start
 * 
 */
    void EvThreadManager::start(){
        for(auto i : _threads){
            i.second->start();
            ROUTN_LOG_INFO(g_logger) << "thread: " << i.first << "start";
        }
    }

/**
 * @brief EvThreadMgr stop
 * 
 */
    void EvThreadManager::stop(){
        for(auto i : _threads){
            i.second->stop();
            ROUTN_LOG_INFO(g_logger) << "thread: " << i.first << "stop";
        }
        for(auto i : _threads){
            i.second->join();
            ROUTN_LOG_INFO(g_logger) << "thread: " << i.first << "join";
        }
    }

    IEvThread::ptr EvThreadManager::get(const std::string& name){
        auto it = _threads.find(name);
        return it == _threads.end() ? nullptr : it->second;
    }

    void EvThreadManager::add(const std::string& name, IEvThread::ptr thr){
        _threads[name] = thr;
    }

}
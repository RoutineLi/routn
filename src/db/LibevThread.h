/*************************************************************************
	> File Name: LibevThread.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年1月19日 星期四 10时38分01秒
 ************************************************************************/

#ifndef __LIBEV_THREAD_H_
#define __LIBEV_THREAD_H_

#include <thread>
#include <vector>
#include <list>
#include <map>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>

#include "../Singleton.h"
#include "../Thread.h"

namespace Routn{

    class EvThread;
    class IEvThread{
    public:
        using ptr = std::shared_ptr<IEvThread>;
        using callback = std::function<void()>;

        virtual ~IEvThread(){};
        virtual bool dispatch(callback cb) = 0;
        virtual bool dispatch(uint32_t id, callback cb) = 0;
        virtual bool batchDispatch(const std::vector<callback>& cbs) = 0;
        virtual void broadcast(callback cb) = 0;

        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void join() = 0;
        virtual void dump(std::ostream& os) = 0;
        virtual uint64_t getTotal() = 0;
    };

    class EvThread : public IEvThread{
    public:
        using ptr = std::shared_ptr<EvThread>;
        using callback = IEvThread::callback;
        using init_cb = std::function<void (EvThread*)>;
        EvThread(const std::string& name = "", struct event_base* base = NULL);
        ~EvThread();

        static EvThread* GetThis();
        static const std::string& GetEvThreadName();
        static void GetAllEvThreadName(std::map<uint64_t, std::string>& names);
    
        void setThis();
        void unsetThis();

        void start();

        virtual bool dispatch(callback cb);
        virtual bool dispatch(uint32_t id, callback cb);
        //批处理回调
        virtual bool batchDispatch(const std::vector<callback>& cbs);
        virtual void broadcast(callback cb);

        void join();
        void stop();
        bool isStart() const { return _start;}

        struct event_base* getBase() { return _base;}
        std::thread::id getId() const;
        void* getData(const std::string& name);
        template<class T>
        T* getData(const std::string& name){
            return (T*)getData(name);
        }
        void setData(const std::string& name, void* v);
        void setInitCb(init_cb v) { _initCb = v;}

        void dump(std::ostream& os);
        virtual uint64_t getTotal() { return _total;}
    private:
        void thread_cb();
        static void read_cb(evutil_socket_t sock, short which, void* args);
    private:
        evutil_socket_t _read;
        evutil_socket_t _write;
        struct event_base* _base;
        struct event* _event;
        std::thread* _thread;
        RW_Mutex _mutex;
        std::list<callback> _callbacks;

        std::string _name;
        init_cb _initCb;

        std::map<std::string, void*> _datas;

        bool _working;
        bool _start;
        uint64_t _total;
    };

    class EvThreadPool : public IEvThread{
    public:
        using ptr = std::shared_ptr<EvThreadPool>;
        using callback = IEvThread::callback;

        EvThreadPool(uint32_t size, const std::string& name = "", bool advance = false);
        ~EvThreadPool();

        void start();
        void stop();
        void join();
        //random thread dispatch
        bool dispatch(callback cb);
        bool batchDispatch(const std::vector<callback>& cbs);

        bool dispatch(uint32_t id, callback cb);

        EvThread* getRandEvThread();
        void setInitCb(EvThread::init_cb v) { _initCb = v;}
        void dump(std::ostream& os);

        void broadcast(callback cb);
        virtual uint64_t getTotal() { return _total;}
    private:
        void releaseEvThread(EvThread* t);
        void check();

        void wrapCb(std::shared_ptr<EvThread>, callback cb);
    private:
        uint32_t _size;
        uint32_t _cur;
        std::string _name;
        bool _advance;
        bool _start;
        RW_Mutex _mutex;
        std::list<callback> _callbacks;
        std::vector<EvThread*> _threads;
        std::list<EvThread*> _freeEvThreads;
        EvThread::init_cb _initCb;
        uint64_t _total; 
    };


    class EvThreadManager{
    public:
        using callback = IEvThread::callback;
        void dispatch(const std::string& name, callback cb);
        void dispatch(const std::string& name, uint32_t id, callback cb);
        void batchDispatch(const std::string& name, const std::vector<callback>& cbs);
        void broadcast(const std::string& name, callback cb);

        void dumpEvThreadStatus(std::ostream& os);

        void init();
        void start();
        void stop();

        IEvThread::ptr get(const std::string& name);
        void add(const std::string& name, IEvThread::ptr thr);
    private:
        std::map<std::string, IEvThread::ptr> _threads;
    };

    using EvThreadMgr = Routn::Singleton<EvThreadManager>;
}


#endif
# 进度

## 日志系统：

    10.22 日志系统 模块开发 实现Logger， Appender， formatter定义               --20%
    10.23 日志系统 模块开发 Logger，Appender， formatter具体实现                --40%
    10.24 日志系统 模块开发 编译调试，cmake 问题，终端cmake版本默认3.0不支
        持最小版本3.0以下， 编译问题显示为基类构造函数未定义引用                    --50%
    10.25 日志系统 模块开发 编译调试，init()解析函数
        中的日志解析pattern子模块调试， 增加一些LOG宏简化使用方法
        编写测试demo--test.cpp测试日志模块                                     --60%
    10.26 日志系统 模块开发 
        (1)新增加Util类，增加LogEventWrap类
        为了不直接使用智能指针LogEvent::ptr，在LogEventWrap实例析构时
        获取_event本身的_logger调用log()写日志                                 --80%

        (2)创建日志管理器（为了简化logger的使用）
            新增类 LogMannger
            数据：
            map<string名称,Logger::ptr> m_logger，用来存放所有logger
            m_root 默认使用的logger
            方法：
            init() 之后会和配置文件联合使用，加载配置产生logger                   --90%
    10.29 日志系统 模块开发
        (1)FileAppender类新增字段bool _time_as_name_needed，从配置文件
        读取后若为true则创建log文件时文件名后缀加上创建日期反之不添加                --91%

## 配置系统：
            
    10.27 配置系统 模块开发 
        (1)ConfigVarBase基类，ConfigVar子类,Config封装实例类及其实现

        (2)YAML研究，yaml-cpp源码阅读，        
        libyaml-cpp安装编译测试
        ```cpp
        YAML::Node node = YAML::LoadFile(filename);
        if(node.Ismap()){
            for(auto it = node.begin(); it != node.end(); ++it){
                it->first, it->second;
            }
        }
        if(node.Issequence()){
            for(auto i = 0; i < node.size(); i++){
                ...
            }
        }                                                                    --60%
    10.28 配置系统 模块开发
        (1)YAML整合，开发基于YAML—CPP库的读取文件接口，实现STL容器，简单
        及复杂类型解析支持,实现日志系统整合配置模块                                                  
        ```cpp
        static Logger::ptr g_log = ROUTN_LOG_NAME("system");
        //当logger的appenders为空，用root写logger
        ```cpp
        定义St_LogDefine St_LogAppenderDefine,并实现Lexicalcast
        相关模板类特化                                                                              
        (2)调试 + debug + 代码格式化                                            --90%
    
## 线程模块：

    10.28 线程封装模块 功能开发         
        (1)Thread线程库pthread接口封装
        (2)互斥锁，CAS锁，读写锁，自选锁封装性能测试调试 //建造者模式
        (3)信号量 Semaphore 封装                                         
        已解决问题：test_thread测试demo中由于是四个线程一起写log
        使用Mutex的话会导致频繁陷入内核，大量的上下文切换占用了cpu时间(写入速度9M/s)
        后续采用自旋锁测试因为自选锁不会让没拿到锁的线程陷入内核而是空转直到拿到锁
        （适用于代码量少，执行语句少的情况比如写log这种）经测试写入性能提高5倍（50M/s）   --70%
    10.29 线程模块封装 功能开发
        (1)配置文件模块整合线程                                                   
        已发现问题：
        1.调试过程中导致demo发生死循环
        -产生原因：ConfigVar类中注册回调函数addListener()读写锁冲突
        -解决办法:为读锁添加额外的作用域使读写分离
        ```cpp
            {//中括号的作用：读写锁分离，离开此括号说明线程完成了读操作，读锁析构，才可以写，
            //否则会出现读锁还未构造，却已经开始写操作
                RWMutexType::ReadLock rlock(_mutex);
                if (v == _temp_val)
                    return;
                for (auto it = _m_callbacks.begin();
                         it != _m_callbacks.end(); ++it)
                {
                    it->second(_temp_val, v);
                }
            }
            RWMutexType::WriteLock wlock(_mutex);
            _temp_val = v;
        ```
        2.调试过程中除上述问题仍导致demo死循环
        -产生原因：FileAppender类的reopen()函数中新增周期写操作
        -解决办法：尚未解决                                                     --90%

## 协程模块：

    10.30  协程模块  模块开发
        (1)assert()宏定义， 方便debug时显示调用栈信息                                 
        (2)Fiber类封装实现,基于协程六态模型
        『
            INIT,   初始化
			HOLD,   保持
			EXEC,   执行
			TERM,   终止
			READY,  就绪
			EXCEPT  未知   』
        采用ucontext接口利用额外的栈空间存储上下文
        已发现问题
        1. 子协程在执行完后没有后续了，没有返回主协程
        -产生原因：Fiber::MainFunc()中执行完任务后并没有swapout切回
        上一个协程
        -解决办法：在Fiber::MainFunc()中末尾执行swapout()
        2. 子协程执行完成后，对应的协程实例并没有发生析构
        -产生原因：子协程执行完后，主协程因为还存在于栈中，导致智能指针的
        引用记数永远不可能小于1
        -解决办法：Fiber::MainFunc()执行完任务后取出当前的协程裸指针
        并reset掉当前的智能指针，用裸指针swapout()                                 --40%
    10.31  协程调度模块 模块开发
          1:N       1:M  
    调度------>thread---->fiber
        1.线程池， 分配一组线程 
        2.协程调度器，将协程部署到相应的线程中                                       --60%
    11.1   协程调度模块 模块开发
       (1) 解决协程调度时上下文切换错误（swapOut()）导致自己的上下文与自己的上下文切换
       陷入了死循环的问题
       (2) 优化了run()函数， 优化了start()函数，并加入多线程调度的相应功能，完善了调度器
       的正常功能                                                               --80%
    11.1   IO协程调度模块 IOManager整合 
```
        IO多路复用使用Linux epoll-------->Schedular类
                        |
                        |
                        v
                    idle (epoll_wait)
                    
        已发现问题：
        1.测试demo中发现会一直陷入在idle函数中的epoll_wait里面
        -产生原因：IOManager里枚举的写事件WRITE赋值错误 0x2==>0x4(EPOLLOUT)
        -解决办法：如上
        2.测试demo中发生断言, 调用栈显示在triggerEvent()中触发
        -产生原因：idle函数中event值没有或上fd_ctx.event
        -解决办法：idle函数中传入triggerEvent()的event要或上fd_ctx.event            --90%
 大致流程：
        大致流程：
            IOManager iom --> IOManager() --> Schedular() --> Fiber() --> schedular.start() //创建io协程调度器实例会初始化线程和线程的协程，此时共有一个线程一个协程
            iom.schedule(callback()) --> iom.tickle() -->                                   //有回调函数或者有协程实例注册绑定回调函数的协程到协程消息队列中(+1)
            ~IOManager() --> Schedular::stop() --> Fiber::call() --> run()                  //在io协程调取器析构时会先执行基类的stop()此方法才是真正开始执行调度逻辑的接口
            -->swapIn() --> Fiber::MainFunc() --> callback() --> addEvent(func)             //run()函数中循环判断该协程绑定的有回调函数则resume该协程并在协程的MainFunc()执行回调函数
            -->swapOut() --> run()                                                          //回调函数中注册事件此时开始执行epoll逻辑，注册完成后返回MainFunc()执行swapOut()回到run
            -->idle()--> IOManager::FdContext::triggerEvent() --> func()                    //run循环判断idle协程状态并执行，idle协程执行idle()并在其中等待epoll_wait()后通过triggerEvent()触发事件
```    
    11.2    IO协程调度模块 定时器模块开发 Timer()
        
        获取当前的定时器触发离现在的时间差
        返回当前需要触发的定时器
        已发现问题：
        1.测试时demo发生死循环
        -产生原因：schedule()函数中循环调用处理时会调用scheduleNolock()，
        此函数中迭代器没有++
        -解决办法：如上
        2.定时器实效，回调只触发了一次就结束了
        -产生原因：定时器的数组中没有添加定时器事件，且getNextTimer()中计算
        下一个定时器时间有错误
        -解决办法：在listExpiredCbs()中遍历timers代码段中遍历完要insert
        ，getNextTimer()返回值要减去当前_ms字段值                                    --99

## HOOK
    
    11.3    Hook模块    对需要Hook的系统函数分类构造使用dlfcn动态库加载函数dlsym
    提取原本的系统函数绑定至函数指针
    文件操作fcntl， ioctl，socket相关，读写相关                                       --60%

    11.4    Hook模块    完善代码逻辑，编写测试demo与sockethook相关                     --99%


## 网络模块 Socket封装

    11.5    网络模块    地址相关封装接口开发
        (1)相关接口实现定义完成开发   --进度60%
        (2)当前本地主机大小端转化库完成开发，可以根据本地主机配置
        来完成大小端转换，利用条件编译和byteswap()                                      --50%
    11.6    网络模块    地址和Socket相关封装接口开发
        (1)相关接口实现定义完成开发   --进度100%
        (2)完成地址转换接口，实现根据获取IP，域名获取IPV4，IPV6地址等信息   
    11.7    网络模块    Socket相关接口封装开发完毕，测试demo调试完成                      --100%              


## ByteArray 序列化
    11.8    完成ByteArray类构造及相关接口封装开发，该类使用了zigzag数据压缩算法，支持从文件读取
    及写入文件以及在编码时可以设置大小端转换, 使用类似链表的结构，新增的数据将放置在链表末尾，当读满
    时会在末尾新增一个节点                                                            --100%

## HTTP协议支持
    11.8    HTTP1.1协议request封装                                                  --50%
    11.18   HTTP解析模块开发
        (1)采用Mongrel2.0的Http解析文件，利用ragel快速搭建生成cpp源文件，并对此生成的源文件封装结合
        (2)HTTP1.1协议response解析器封装                                             --100%
    11.20   TCPServer模块开发   接口封装                                             --100%

## Stream流封装
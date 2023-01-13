# By Routnli feat. sylar
学习交流用，基于sylar框架的仿写，旨在提升自己的架构思维，编码水平以及独立开发项目的能力

## 日志系统 
1) 
    logger(定义日志类别)
    |
    |-----------------formatter(日志格式: 业务, error, warning, debug, fatal)
    |
    Appender(日志输出目录)

## 配置系统 
1)
    # yamlcpp库安装 
        yml格式配置文件的读取和整合
    # boost库安装-----------使用到lexical_cast转换
        lexical_cast 是依赖于字符串流 std::stringstream 的，其原理也是相当的简单：把源类型 (Source) 读入到字符流中，再写到目标类型 (Target) 中。但这里同时也带来了一些限制： 
        - 输入数据 (arg) 必须能够 “完整” 地转换，否则就会抛出 bad_lexical_cast 异常

    # 原则：约定优于配置思想:
```cpp
    template<T, FromStr, ToStr>
    class ConfigVar;
    
    template<F, T>
    LexicalCast;
    
    //容器片特化模板类
    //vector list map unordered_map set unordered_set
```
    自定义类型，需要实现Routn::LexicalCast,片特化实现后，就可以支持Config解析自定义类型并支持与
    规定stl容器嵌套使用
    
    配置的事件机制：
        当一个配置项发生修改的时候，可以反向通知对应代码
    # 日志系统整合配置系统
```yaml
    logs:
        - name: root
          level: info
          formatter: "%d%T%m%n"
          appender:
              - type: FileAppender
                  file: ../../log/root_log.txt
              - type: StdoutAppender
```

## 线程库封装
    Thread, Mutex, condition_variable, semaphore, thread_pool
    pthread_create
    std::thread->pthread---没有区分读写锁，不太适用高并发
    选型：threads=>pthread
       mutex=>pthread_mutex
       cond=>pthread_cond
       sem=>semaphore
       rwmutex=> pthread_rwlock_t
       spinlock=> pthread_spinlock_t
       rw_spinlock=> Intel->tbb::rw_spinlock
    线程库整合：对于语句少，只负责写的日志模块采用SpinLock
              对于配置模块，读多写少，采用读写分离的读写锁RW_Mutex

​		需要第三方库TBB

```
	pthread中提供的锁有：pthread_mutex_t, pthread_spinlock_t, pthread_rwlock_t。
pthread_mutex_t是互斥锁，同一瞬间只能有一个线程能够获取锁，其他线程在等待获取锁的时候会进入休眠状态。因此pthread_mutex_t消耗的CPU资源很小，但是性能不高，因为会引起线程切换。
pthread_spinlock_t是自旋锁，同一瞬间也只能有一个线程能够获取锁，不同的是，其他线程在等待获取锁的过程中并不进入睡眠状态，而是在CPU上进入“自旋”等待。自旋锁的性能很高，但是只适合对很小的代码段加锁（或短期持有的锁），自旋锁对CPU的占用相对较高。
pthread_rwlock_t是读写锁，同时可以有多个线程获得读锁，同时只允许有一个线程获得写锁。其他线程在等待锁的时候同样会进入睡眠。读写锁在互斥锁的基础上，允许多个线程“读”，在某些场景下能提高性能。
    
·单线程不加锁：0.818845s
·多线程使用pthread_mutex_t：120.978713s (很离谱吧…………我也吓了一跳)
·多线程使用pthread_rwlock_t：10.592172s （多个线程加读锁）
·多线程使用pthread_spinlock_t：4.766012s
·多个线程使用tbb::spin_mutex：6.638609s （从这里可以看出pthread的自旋锁比TBB的自旋锁性能高出28%）
·多个线程使用tbb::spin_rw_mutex：3.471757s （并行读的环境下，这是所有锁中性能最高的）

```


## 协程库封装(支持LibCo和Ucontext两种接口实现)

    采用有栈协程的方式(stackfull)实现
    ucontext_t.
    macro
```
    Thread->main_fiber <------------> sub_fiber
                |
                |
                v
              sub_fiber
    非对称协程模型:子协程只能返回到上一个协程, 子协程之间不能相互切换
```
### 协程间同步----FiberSemaphore
    我们都知道，一旦协程阻塞后整个协程所在的线程都将阻塞住，这也就失去了协程的优势。而随着业务的升级迭代，协程间同步甚至协程间通信的概念便被提出，原本的LibCo并不支持协程同步原语，而Go语言之所以能再短短几年称霸后端语言一方，迅速占有市场，不但因为其内部完整实现了Go协程——goroutine的同步机制，还实现了强大的协程间通信——Channel。
    
    ​	然而Linux常见的提供给我们的同步原语和互斥量都会发生阻塞，因此需要重新实现一套用户态协程同步原语才能解决问题。
    
    ​	如同协程比于线程，我们很容易得到一个启示，既然内核维护等待队列会阻塞线程，那可以用户态自己维护，当获取协程同步对象失败时，用户将条件不满足的协程放入队列，然后Yield让出控制权，等待条件满足时再将协程重新从队列取出加入调度器继续执行，与HOOK模块的原理非常相似！
    先来看一看FiberSemaphore的成员变量:
```cpp
    class FiberSemaphore{
    public:
        trywait();
        wait();
        notify();
    private:
        MutexType m_mutex;          //自旋锁，不会阻塞线程
        std::list<std::pair<Scheduler*, Fiber::ptr> > m_waiters; //协程对象和当前协程所属调度器的对象
        size_t m_concurrency;       //并发量(信号量值)
    };
```
    由此不难看出协程信号量的本质就是维护了一个协程当前上下文的队列，执行wait()操作时并发量-1,如果并发量为0则将当前执行协程压入队列并让出当前协程。执行notify()操作时并发量+1并重新调度队首协程的上下文。

## 协程调度模块 
    Schedular基类====>IOManager基于IO特化的协程调度器
    schedule(func/fiber)
    
    start()
    stop()
    run()                                               //会创建额外两个协程，执行回调的协程，idle协程
    
    1.设置当前线程的schedular
    2.设置当前线程的run，fiber
    3.协程调度循环while(1)
```    
        1.协程消息队列里面是否有任务
        2.没有任务执行idle

        IO多路复用使用Linux epoll-------->Schedular类
                        |
                        |
                        v
                    idle (epoll_wait)

           信号量 Sem_t wait(), post()
        
            post +1
        message_queue
            |
            |-----Thread1
            |------Thread2
                -RecvMsg(msg) wait -1

    异步IO，等待数据返回-->epoll_wait等待
```

```
大致流程：
    IOManager iom --> IOManager() --> Schedular() --> Fiber() --> schedular.start() 创建io协程调度器实例会初始化线程和线程的协程，此时共有一个线程一个协程
    iom.schedule(callback()) --> iom.tickle() -->                                   有回调函数或者有协程实例注册绑定回调函数的协程到协程消息队列中(+1)
    ~IOManager() --> Schedular::stop() --> Fiber::call() --> run()                  在io协程调取器析构时会先执行基类的stop()此方法才是真正开始执行调度逻辑的接口
    -->swapIn() --> Fiber::MainFunc() --> callback() --> addEvent(func)                 run()函数中循环判断该协程绑定的有回调函数则resume该协程并在协程的MainFunc()执行回调函数
    -->swapOut() --> run()                                                          回调函数中注册事件此时开始执行epoll逻辑，注册完成后返回MainFunc()执行swapOut()回到run
    -->idle()--> IOManager::FdContext::triggerEvent() --> func()                    run循环判断idle协程状态并执行，idle协程执行idle()并在其中等待epoll_wait()后通过triggerEvent()触发事件

            [Fiber]                    [Timer]
                ^  N                      ^
                |                         |
                | 1                       |
            [Thread]                [TimerManager]
                ^  M                      ^
                |                         |
                | 1                       |
            [Schedular] <---------[IOManager(epoll)]
```
## 定时器模块
    本协程库的定时器设计基于最小堆，本质就是每次从定时器Set中取出一个绝对时间最小的来与当前绝对时间判断，若
    
    超时则执行，支持最高精度Ms级（Epoll最高支持ms级）, 支持添加，删除，条件触发，循环触发定时器事件。实现较为简单。
    
    ​	定时事件依赖于Linux提供的定时机制，目前Linux提供了以下几种可供我们使用的机制:
    
    ```
    1. 套接字超时选项，SO_RCVTIMEO和SO_SNDTIMEO,通过errno判断超时
    2. alarm()和settimer()，通过SIGALRAM信号触发，通过sighandler捕获信号判断超时
    3. IO多路复用epoll的超时选项
    4. timerfd_create()和timer_create()系列接口
    ```
    ​	本定时器基于epoll_wait超时机制，IO调度器会在调度器空闲时阻塞在执行epoll_wait()的协程上，等待IO事件发生，epoll_wait()的超时依赖于当前定时器的最小超时时间，每次epoll_wait()返回后都会根据当前的绝对时间把已经超时的定时器事件收集起来，执行它们的回调函数，由于epoll_wait()的返回值不能绝对判断是否超时，因此每次返回后都要再判断一下定时器有没有超时，这时通过比较当前绝对时间和定时器的绝对超时时间，就可以确定一个定时器是否超时。
    
    ​	由于本定时器采用gettimeofday()获取绝对时间，所以依赖于系统时间，如果系统时间被修改或者校时，那么这套定时机制就会失效，目前解决方法是每次默认设置一个较小的epoll_wait()超时时间，比如3s，假设有一个定时任务超时时间是10s以后，那么epoll_wait()要超时三次才能触发，每次超时后还要检查一下系统时间是否被回调过，如果被回调一个小时以上，则默认全部超时（这个办法还比较暴力，更好的解决办法是换一个时间源，改为clock_gettime(CLOCK_MONOTONIC_RAW, &ts)获取单调时间更好一点）。

## HOOK
    	由于协程是用户态下的线程，在日常业务处理中，一旦协程所在的函数有阻塞，那么整个协程所在的线程都会阻塞在这里，这会大大降低应用协程的性能，甚至会发生意想不到的重大事故，例如死锁。因此在异步化改造过程中使用原版的各种系统调用像是accept(), connnect(), sleep()等等都会产生陷入内核态从而导致阻塞，那该怎么办呢？聪明的程序员想到了一个办法，通过将各种会产生阻塞的系统调用做一个hook，将原版系统调用替换一下，让代码执行我们想要的非阻塞操作不就好了吗，例如sleep()函数休眠5s，正常情况下调用线程会陷入内核态阻塞住5s，但hook后，线程并不会真的阻塞而是让协程添加一个5s的定时器任务，然后将该协程yield让出执行权，线程上的其它协程继续执行，等到5s后定时器触发，再接着执行原本的上下文。这样子给人感觉像是阻塞住了，实际上通过定时器巧妙的实现了和原版系统调用相同的效果并且没有阻塞线程，这就是Hook模块的作用。

## socket库封装

    支持IPv4,IPV6,domain,unix路径绑定地址创建套接字, 基于OpenSSL库，支持
    SSL协议握手
``` 
                   [UnixAddress]-[path address]-[unix socket]
                        |
                    ---------                     |-[ipv4]
                   | address | --- [ip address ]--|
                    ---------                     |-[ipv6]
                        |                         
                        |        
                        |         
                        |
                    ---------
                    | socket |
                    ---------
```

## 序列化协议(Zigzag)--ByteArray模块
    数据结构本质是一个链表，每个链表节点存储基于Zigzag编码后的二进制数据。
    
    系统之间进行通讯的时候，往往又需要以整型（int）或长整型（long）为基本的传输类型，他们在大多数系统中，以4Bytes和8Bytes来表示。这样，为了传输一个整型（int）1，我们需要传输00000000_00000000_00000000_00000001 32个bits，除了一位是有价值的1，其他全是基本无价值的0
    zigzag给出了一个很巧的方法：我们之前讲补码讲过，补码的第一位是符号位，他阻碍了我们对于前导0的压缩，那么，我们就把这个符号位放到补码的最后，其他位整体前移一位：
```
        (-1)10

        = (11111111_11111111_11111111_11111111)补

        = (11111111_11111111_11111111_11111111)符号后移

        但是即使这样，也是很难压缩的，因为数字绝对值越小，他所含的前导1越多。于是，这个算法就把负数的所有数据位按位求反，符号位保持不变，得到了这样的整数：

        (-1)10

        = (11111111_11111111_11111111_11111111)补

        = (11111111_11111111_11111111_11111111)符号后移

        = (00000000_00000000_00000000_00000001)zigzag

        而对于非负整数，同样的将符号位移动到最后，其他位往前挪一位，数据保持不变。

        (1)10

        = (00000000_00000000_00000000_00000001)补

        = (00000000_00000000_00000000_00000010)符号后移

        = (00000000_00000000_00000000_00000010)zigzag

        这两种case，合并到一起，就可以写成如下的算法：

        整型值转换成zigzag值：

        int int_to_zigzag(int n)

        {

                return (n <<1) ^ (n >>31);

        }
        n << 1 :将整个值左移一位，不管正数、0、负数他们的最后一位就变成了0；

    解码的方法：

        zigzag用字节自己表示自己。具体怎么做呢？我们来看看代码：

        int write_to_buffer(int zz,byte* buf,int size)

        {
                int ret =0;
                for (int i =0; i < size; i++)
                {  
                        if ((zz & (~0x7f)) ==0)
                        {
                                buf[i] = (byte)zz;
                                ret = i +1;
                                break;
                        }
                        else
                        {
                                buf[i] = (byte)((zz &0x7f) |0x80);
                                zz = ((unsignedint)zz)>>7;
                        }
                }
                return ret;
        }   
        大家先看看代码里那个(~0x7f)，他究竟是个什么数呢？
        (~0x7f)16
        
        =(11111111_11111111_11111111_10000000)补

        他就是从倒数第八位开始，高位全为1的数。他的作用，就是看除开最后七位后，还有没有信息。

        我们把zigzag值传递给这个函数，这个函数就将这个值从低位到高位切分成每7bits一组，如果高位还有有效信息，则给这7bits补上1个bit的1（0x80）。如此反复 直到全是前导0，便结束算法。

        举个例来讲：

        (-1000)10
        = (11111111_11111111_11111100_00011000)补
        = (00000000_00000000_00000111_11001111)zigzag

        我们先按照七位一组的方式将上面的数字划开：
        (0000-0000000-0000000-0001111-1001111)zigzag
        
        A、他跟(~0x7f)做与操作的结果，高位还有信息，所以，我们把低7位取出来，并在倒数第八位上补一个1(0x80)：11001111
        
        B、将这个数右移七位：(0000-0000000-0000000-0000000-0001111)zigzag       

        C、再取出最后的七位，跟(~0x7f)做与操作，发现高位已经没有信息了（全是0），那么我们就将最后8位完整的取出来：00001111，并且跳出循环，终止算法；
        
        D、最终，我们就得到了两个字节的数据[11001111, 00001111]

```

## http模块
支持HTTP/1.1 --API

HTTPRequest;
HTTPResponse;
HTTParser --> ragel --> github-->mongrel2/src/http11

http_parser.h 模块利用了 Ragel 来解析 http 协议报文。
Ragel 是个有限状态机编译器，它将基于正则表达式的状态机编译成传统语言（C，C++，D，Java，Ruby 等）的解析器。Ragel 不仅仅可以用来解析字节流，它实际上可以解析任何可以用正则表达式表达出来的内容。而且可以很方便的将解析代码嵌入到传统语言中。
http_parser.h 模块利用了别人写好的按照 ragel 提供的语法与关键字编写的 .rl 文件，从中拷贝出 3 个头文件，2 个 rl 文件。其中 .c 文件是通过 rl 文件生成的。
具体地址如下：
https://github.com/mongrel2/mongrel2/tree/master/src/http11
具体生成 .cc 文件的指令如下：

```
        $ ragel -v			// 获取版本号
        $ ragel -help		// 获取帮助文档

        $ ragel -G2 -C http11_parser.rl -o http11_parser.cc
```
HTTP.h 中 HttpRequest 和 HttpResponse 的 dump 方法里面组装出来的字符串其实就是 http 报文了。
关于 HTTP 请求和响应的格式可参考如下地址：
https://developer.mozilla.org/zh-CN/docs/Web/HTTP/Messages

## TCP-Server 模块
封装出一个独立的Tcp的服务端模块
可以基于该模块派生支持自定义业务功能
实例代码包含一个派生出来的简单的EchoServer

## Stream  针对文件/Socket封装
    read/write/readFixSize/WriteFixSize
    
    HttpSession/HttpConnection
    Server.accept, socket->session
    Client connect socket->connection
### ZLIBStream 
继承自Stream基类，封装了一套Zlib解析读写stream接口以便支持HTTP协议提供的数据压缩(gzip, inflate...)

### SocketStream

继承自Stream基类和Socket基类，封装了Socket相关IO操作的读写流式接口，支持字符串流和二进制流(基于ByteArray模块提供的序列化方案)

### AsyncSocketStream
继承自SocketStream类特化的异步Socket流式I/O接口，由于大部分主流Linux不支持类似于io_cp的纯异步IO(更高版本的内核已经实现了更为高效的io_uring),所以结合协程信号量和读写锁以及IO调度模块实现一个异步流式Socket接口。
先来看看成员变量：

```cpp	
class AsyncSocketStream : public SocketStream
                        : public std::enabe_shared_from_this<AsyncSocketStream>{
public:
    struct SendCtx;                         //上下文基类
    struct Ctx : public SendCtx{
    	uint32_t sn;                        //上下文序列号
		uint32_t timeout;                   //超时时间
		uint32_t result;                    //结果
		bool timed;                         //是否超时

		Schedular* scheduler;               //处理该上下文对应的调度器
		Fiber::ptr fiber;                   //处理该上下文所属的协程
		Timer::ptr timer;                   //定时器
    };           //上下文
protected:
    Routn::FiberSemaphore _sem;                     //维护消息队列操作信号量
	Routn::FiberSemaphore _waitSem;                 //控制读写协程的信号量
	RWMutexType _queueMutex;                        //消息队列锁
	RWMutexType _mutex;                             //Stream锁
	std::list<SendCtx::ptr> _queue;                 //消息队列
	std::unordered_map<uint32_t, Ctx::ptr> _ctxs;   //上下文映射 sn->ctx
	uint32_t _sn;                                   //序列号
	bool _autoConnect;                              //是否自动连接
	Routn::Timer::ptr _timer;                       //定时器负责自动连接
	Routn::IOManager* _ioMgr;                       //调度器
	Routn::IOManager* _worker;

	connect_callback _connectCB;                    //连接时的回调，参数为异步当前Stream
	disconnect_callback _disconnectCB;              //断开连接的回调

	boost::any _data;                               //any_type data
};
```
由此可见该流式接口封装了一个信号量来对做读写的两个协程进行同步，调用该流的start()方法时首先会waitFiber()消费掉读和写的信号量然后依次调用connect_callback和disconnect_callback，之后判断是否断开自动连接，并注册重连事件到定时器中，最后开始分别开始执行读写操作两个协程。
(1) 读操作: 

```cpp
    void AsyncSocketStream::doRead(){
        ...
        while(isConnected()){
            Ctx::ptr ctx = doRecv();    //recv data==>Ctx
            if(ctx)
                ctx->doRsp();           //make response
        }
        innerClose();                   //关闭流
        _waitSem.notify();              //++读信号量
        ...
    }
```
(2) 写操作：

```cpp
    void AsyncSocketStream::doWrite(){
        ...
        /**
         * 实际上基于协程信号量的实现，消息队列每次只会有一个
         * 消息，保证了每次只会取出一个消息去消费，原子性。
         **/
        while(isConnected()){
            _sem.wait();               //--队列信号量
            std::list<SendCtx::ptr> ctxs;
            {
                WriteLock lock(_queueMutex);
                _queue.swap(ctxs)   //获取_queue的值，语义权转换，防止智能指针失效
            }
            for(auto &i : ctxs){    //send Message;
                //doSend
                if(!i->doSend(std::shared_from_this())){
                    innerClose();
                    break;
                }
            }
        }
        {
            WriteLock lock(_queueMutex);
            _queue.clear();         //清空队列
        }
        _waitSem.notify();          //++写信号量
    }
```
(3) 消息队列入队:

```cpp
    bool AsyncSocketStream::enqueue(SendCtx::ptr ctx){
        ...
        WriteLock lock(_queueMutex);
        bool rt = _queue.empty();   
        _queue.push_back(ctx);  //入队
        lock.unlock();
        if(rt)              //当且仅当上一个消息消费完了才会++信号量
            _sem.notify();
        return rt;
    }
```

## Servlet模块

    Servlet <-------- FunctionServlet
        |
        |
        v
  ServletDispatch

## HTTP-Conncection模块
封装http协议发送请求和响应及其他实现
基于SocketStream基类
通过SocketStream一致化
实现Http-keepAlive

## 守护进程
    deamon,
```cpp
    fork
     | ------子进程执行server
     | ------父进程调用wait（pid）收尸
```
## 输入参数解析
    int argc, char** argv
    
    ./routn -d -c conf

   # 环境变量
   getenv
   setenv
   /proc/pid/cmdline

   利用/proc/pid/cmdline
   和全局构造函数实现在进入main函数前解析参数

   1.读写环境变量
   2.获取程序的绝对路径，基于绝对路径设置cwd
   3.可以通过cmdline，在进入main函数之前，解析好参数

   # 配置加载
   配置的文件夹路径：log.yml, http.yml, tcp.yml, redis.yml...等等

## 配置加载模块
```cpp
   void LoadFromConf(conf);
    //读取对应目录里的全部yaml文件，并更新应用在服务中
```

## Module模块化
```
    通过动态库导出的"CreateModule"接口实例化业务模块
                |
                v
    Application类会在init()中调用每个Module的onServerReady()
                |
                v
    Module的onServerReady注册好业务模块自己的Servlet
                |
                v
    Application初始化完毕，server开始绑定socket
                |
                v
    在业务模块自己的Servlet.handleClient()中实现业务    
```

## WebSocket模块
基于HTTP模块开发，实现了WebSocketServer, WebSocketServlet, WebSocketSession, WebSocketConnection
实现了解析WebSocket协议，WebSocket握手，秘钥交换，处理会话等功能。

```cpp
    //WebSocket协议头
    struct WSFrameHead{
		enum OPCODE{
			/// 数据分片帧
			CONTINUE =  0,
			/// 文本帧
			TEXT_FRAME = 1,
			/// 二进制帧
			BIN_FRAME = 2,
			/// 断开连接
			CLOSE = 8,
			/// PING
			PING = 0x9,
			/// PONG
			PONG = 0xA
		};
		uint32_t opcode: 4;			//4位opcode
		bool rsv3: 1;					//1位rsv3
		bool rsv2: 1;					//1位rsv2
		bool rsv1: 1;					//1位rsv1
		bool fin: 1;					//1位FIN标志位
		uint32_t payload: 7;			//7位Payload
		bool mask: 1;					//1位mask掩码
	};
```

## SQLite3接口
    支持SQlite3Stmt查询。
## Redis接口
    //TODO
## MYSQL接口
    //TODO
## ORM模块
    //TODO

## ZKClient模块
基于zookeeper3.70/zookeeper-client-c源码封装的zookeeper客户端模块，方便框架通过ZK客户端对分布式场景进行监控，协调，处理(负载均衡，熔断降级，服务发现)

## 分布式rpc模块

### 自定义分布式Rock协议

​	Rock协议-----底层采用ByteArray + Protobuf进行序列化，三种消息类型，REQUEST，RESPONSE， NOTIFY， 支持g-zip压缩(ZLIBStream)

#### 协议头设计：

``` cpp
 struct RockMsgHeader{
    uint8_t magic[2];  //魔法数
    uint8_t version;   //版本号
    uint8_t flag;    //flag标志位
    int32_t length;   //长度
  };
```

#### 消息体：继承自Protocol ==> Message基类

```
Request===>{
	_sn: 序列号
	_cmd: 状态码
    _body: 消息体
}

Response===>{
	_sn: 序列号
	_cmd: 状态码
	_result: 响应码
	_resultStr: 响应原因
    _body: 响应消息体
}

Notify===>{
	_notify: 通知序列号
    _body: 通知消息体
}
```

### RockStream 流式接口
基于AsyncSocketStream流接口，重载了消息上下文SendCtx, Ctx==>RockSendCtx, RockCtx, 重载的上下文内置了Rock协议的请求响应数据结构

request()函数：负责主动发送请求到对端，纯异步，返回一个RockResult结构(本质上是对Ctx消息上下文的包装)。
RockResult结构如下：

```cpp
struct RockResult{
    int32_t result;
    int32_t used;
    RockResponse::ptr response;
    RockRequest::ptr request;
};
```

### 服务发现--------基于Zookeeper

 在一个分布式环境下，服务方法的调用端怎么知道自己调用的服务方法在哪台机器上有提供？怎么获得这个机器的IP地址和端口以联系这个器并请求服务？所以这里就需要注册中心集群了，注册中心里面会记录服务对象方法以及提供者的IP端口。这个获取服务对象方法提供者地址信息的过程就叫服务发现！
 服务发现机制主要提供两个功能！

 1. 服务注册：服务提供方在正式提供服务之前要先把自己的服务对象注册到注册中心上，注册中心会把这个服务提供者的地址信息以及提供的服务对象名+方法名保存下来。
 2. 服务订阅：在服务调用方启动时，会去注册中心找自己需要的服务对象对应的服务提供者的地址信息，然后缓存到本地，为远程调用做储备。

本框架实现基于ServiceItemInfo=>基于服务项目的上下文和IServiceDiscovery服务发现基类
先来看看这两个结构的定义：
```cpp
    class ServiceItemInfo{
    public:
        static Create(...);     //静态方法构建服务上下文结构
    private:
        uint64_t m_id;          //上下文id
        uint16_t m_port;        //服务端口
        std::string m_ip;       //服务的ip地址
        std::string m_data;     //服务存储的数据-----服务类型(rock, grpc, redis, namesever...)
    };
    
    /***
     * 提供服务查询，服务注册，设置服务回调(服务提供的方法, 远程调用)
     **/
    class IServiceDiscovery{
    protected:
        //domain -> [service -> [id -> ServiceItemInfo] ]         域名->[服务->[服务上下文id->上下文]]
        std::unordered_map<std::string, std::unordered_map<std::string
            ,std::unordered_map<uint64_t, ServiceItemInfo::ptr> > > m_datas;
        //domain -> [service -> [ip_and_port -> data] ]           域名->[服务->[地址->data]]
        std::unordered_map<std::string, std::unordered_map<std::string
            ,std::unordered_map<std::string, std::string> > > m_registerInfos;
        //domain -> [service]                                     域名->[服务]
        std::unordered_map<std::string, std::unordered_set<std::string> > m_queryInfos;

        std::vector<service_callback> _cbs;				//服务对应的回调函数组
       	
        std::string _selfInfo;						//该服务发现的信息
        std::string _selfData;						//该服务发现的数据
        
        std::map<std::string, std::string> _params;	//参数映射
    };

```
ZKServiceDiscovery类-->继承自IServiceDiscovery类，实现了start(),stop()方法, 底层基于ZKClient模块实现，基于zookeeper完成了服务发现，服务注册，更新通知等功能。
节点设计为生产者和消费者两类
生产者：根路径/providers/...
消费者：根路径/consumers/...
来看看比较重要的几个函数:

```cpp
/// ZKClient 监视回调函数onWatch
void ZKServiceDiscovery::onWatch(int type, int stat, const std::string& path, 									ZKClient::ptr client){
		if(stat == ZKClient::StateType::CONNECTED){
			if(type == ZKClient::EventType::SESSION){
				return onZKConnect(path, client);
			}else if(type == ZKClient::EventType::CHILD){
				return onZKChanged(path, client);
			}else if(type == ZKClient::EventType::CHANGED){
				return onZKChanged(path, client);
			}else if(type == ZKClient::EventType::DELETED){
				return onZKDeleted(path, client);
			}
		}else if(stat == ZKClient::StateType::EXPIRED_SESSION){
			if(type == ZKClient::EventType::SESSION){
				return onZKExpiredSession(path, client);
			}
		}
		ROUTN_LOG_ERROR(g_logger) << "onWatch hosts = " << _hosts
			<< " type = " << type << " stat = " << stat
			<< " path = " << path << " client = " << client << " is invalid";
	}

/// 服务注册
bool ZKServiceDiscovery::registerInfo(const std::string& domain
                                      , const std::string& service
                                      , const std::string& ip_and_port
                                      , const std::string& data){
    std::string path = GetProvidersPath(domain, service);	//获取域名根路径
    bool v = existsOrCreate(path);							//判断该路径是否存在
    if(!v){
        ROUTN_LOG_ERROR(g_logger) << "create path = " << path << " fail";
        return false;
    }

    std::string new_val(1024, 0);
    //创建一个临时节点
    int32_t rt = _client->create(path + "/" + ip_and_port, data, new_val
                                 , &ZOO_OPEN_ACL_UNSAFE, ZKClient::FlagsType::EPHEMERAL);
    if(rt == ZOK){
        return true;
    }
    if(!_isOnTimer){
        ROUTN_LOG_ERROR(g_logger) << "create path = " << (path + "/" + ip_and_port) << "fail, error:"
            << zerror(rt);
    }
    //判断节点是否已经添加成功
    return rt == ZNODEEXISTS;
}

///start启动ZKServiceDiscovery
void ZKServiceDiscovery::start(){
    if(_client){
        return ;
    }
    auto self = shared_from_this();
    _client.reset(new Routn::ZKClient);
    //ZKClient注册onWatch回调
    bool b = _client->init(_hosts, 6000, std::bind(&ZKServiceDiscovery::onWatch
                                                   , self, std::placeholders::_1, 													std::placeholders::_2, 												std::placeholders::_3, std::placeholders::_4));
    if(!b){
        ROUTN_LOG_ERROR(g_logger) << "ZKClient init fail, hosts = " << _hosts;
    }
    //定时任务每分钟强制刷新一次(强一致性)
    _timer = IOManager::GetThis()->addTimer(60 * 1000, [self, this](){
        _isOnTimer = true;
        onZKConnect("", _client);
        _isOnTimer = false;
    }, true);
}
```

使用Zookeeper作为分布式服务的注册中心缺点也是它的优点，那便是强一致性。

ZooKeeper最大特点就是强一致性，只要ZooKeeper上面有一个节点发生了更新，都会要求其他节点一起更新，保证每个节点的数据都是完全实时同步的，在所有节点上的数据没有完全同步之前不干其他事。
 拿ZooKeeper搞点小项目其实还能应对，但是如果分布式环境中提供服务的和访问服务的机器越来越多，变化越来越频繁时，ZooKeeper为了维持这个强一致性需要付出很多代价，最后ZooKeeper服务注册中心会承受不住压力而崩溃。



### 负载均衡

​	本框架负载均衡提供了FAIR，ROUNDROBIN， WEIGHT三种策略

大体结构分为：

```
				[LoadBalance]————————————————>[ServiceDiscoveryLoadBalance]
					  |							特化的服务发现负载均衡
					  |1:n
					  |
					  v
			 [id ,[LoadBalanceItem]]
			 		  |
			 		  |n:m
			 		  |
			 		  v
			[[HolderStatsSet], [....]]
					  |
					  |
					  |
					  v
			[[HolderStats], [HolderStats], [HolderStats]....]
```

#### HolderStats结构：

​	这个结构存储了当前服务的各项数据，并提供了对这些数据的原子操作;

```cpp
struct HolderStats{
    ...
private:
		uint32_t _usedTime = 0;				//服务总时间
		uint32_t _total = 0;				//总连接数
		uint32_t _doing = 0;				//该服务正在使用
		uint32_t _timeouts = 0;				//超时数
		uint32_t _oks = 0;					//成功数
		uint32_t _errs = 0;					//失败数
};
```

#### HolderStatsSet：

​	这个结构提供了当前服务的上次更新时间和一个HolderStats数组，以及基于HolderStats计算当前服务的权重的函数。

```cpp
权值计算方法：
    (1) 通过以下算法得出一个HolderStats权重
float getWeight(float rate){
		float base = _total + 20;
		return std::min((_oks * 1.0 / (_usedTime + 1)) * 2.0, 50.0)
			* (1 - 4.0 * _timeouts / base)
			* (1 - 1 * _doing / base)
			* (1 - 10.0 * _errs / base) * rate;        
}
   （2） 根据_lastUpdate自段与当前时间戳比较，若比当前时间戳早，则通过_stats[_lastUpdate % _stats.size()]的方法清空掉时间戳之前的数据.
void init(uint32_t& now){
    		if(_lastUpdateTime < now){
			for(uint32_t t = _lastUpdateTime + 1, i = 0; 
					t <= now && i < _stats.size(); ++t, ++i){
				_stats[t % _stats.size()].clear();
			}
			_lastUpdateTime = now;
		}
 }
    将当前时间戳赋给_lastUpdate
    （3） init()之后遍历_stats数组，按照加和一次加上每一项的权重
float getWeight(const uint32_t& now){
		init(now);
        float v = 0;
		for(size_t i = 1; i < _stats.size(); ++i){
			v += _stats[(now - i) % _stats.size()].getWeight(1 - 0.1 * i);
		}
		return v;        
}

```


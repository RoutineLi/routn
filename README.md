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
    
2)
    json xml

## 线程库封装
    Thread, Mutex, condition_variable, semaphore, thread_pool
    pthread_create
    std::thread->pthread---没有区分读写锁，不太适用高并发
    选型：threads=>pthread
	   mutex=>pthread_mutex
	   cond=>pthread_cond
	   sem=>semaphore
    线程库整合：对于语句少，只负责写的日志模块采用SpinLock
              对于配置模块，读多写少，采用读写分离的读写锁RW_Mutex

## 协程库封装(支线:尝试采用libco)
    采用ucontext接口(unix Posix)
    ucontext_t.
    macro
```
    Thread->main_fiber <------------> sub_fiber
                |
                |
                v
              sub_fiber


    非对称协程模型:子协程只能返回到上一个协程, 子协程之间不能相互切换
    Schedular
    schedule(func/fiber)

    start()
    stop()
    run()                                               //会创建额外两个协程，执行回调的协程，idle协程

    1.设置当前线程的schedular
    2.设置当前线程的run，fiber
    3.协程调度循环while(1)
    
        1.协程消息队列里面是否有任务
        2.没有任务执行idle

        IO多路复用使用Linux epoll-------->Schedular类
                        |
                        |
                        v
                    idle (epoll_wait)

           信号量
        PutMsg(msg,) post +1
        message_queue
            |
            |-----Thread1
            |------Thread2
                -RecvMsg(msg) wait -1

    异步IO，等待数据返回-->epoll_wait等待

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
## HOOK
    使用Hook封装常用的系统库函数，实现同步实现异步
    sleep
    usleep
    目的：Hook的原理就是利用定时器和IOManager实现原系统函数并替换，例如sleep() 系统的sleep会阻塞线程，从而导致性能下降，
    而实现的hook版本不会阻塞当前线程，使用定时器唤醒的机制调度协程来实现与原本sleep()相同的效果

## socket库封装

``` 
                   [UnixAddress]
                        |
                    ---------                     |-[ipv4]
                   | address | --- [ip address ]--|
                    ---------                     |-[ipv6]
                        |
                        |
                    ---------
                    | socket |
                    ---------
                    
```
## 序列化协议(Zigzag)
    而我们在系统之间进行通讯的时候，往往又需要以整型（int）或长整型（long）为基本的传输类型，他们在大多数系统中，以4Bytes和8Bytes来表示。这样，为了传输一个整型（int）1，我们需要传输00000000_00000000_00000000_00000001 32个bits，除了一位是有价值的1，其他全是基本无价值的0
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

## http协议解析
    支持HTTP/1.1 --API
    
HTTPRequest;
HTTPResponse;
HTTPParser --> ragel --> github-->mongrel2/src/http11

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
    http.h 中 HttpRequest 和 HttpResponse 的 dump 方法里面组装出来的字符串其实就是 http 报文了。
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

    LoadFromConf();


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

## 分布式协议

## WebSocket协议支持
   基于Http模块派生出的Websocket协议模块，支持串行化，Websocket协议解析，握手的密钥算法基于openssl的base64和sha1sum，详情请见Util.cpp

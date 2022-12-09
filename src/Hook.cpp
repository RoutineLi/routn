/*************************************************************************
	> File Name: Hook.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月03日 星期四 17时34分39秒
 ************************************************************************/

#include "Hook.h"
#include "Macro.h"
#include "Config.h"

namespace Routn
{
	static thread_local bool t_hook_enable = false;
	static Routn::ConfigVar<int>::ptr g_tcp_connect_timeout = 
		Routn::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

#define HOOK_FUN(x) \
	XX(sleep)       \
	XX(usleep)      \
	XX(nanosleep)   \
	XX(socket)      \
	XX(connect)     \
	XX(accept)      \
	XX(getsockopt)  \
	XX(setsockopt)  \
	XX(read)        \
	XX(readv)       \
	XX(recv)        \
	XX(recvfrom)    \
	XX(recvmsg)     \
	XX(write)       \
	XX(writev)      \
	XX(send)		\
	XX(ioctl)    	\
	XX(sendto)      \
	XX(sendmsg)     \
	XX(close)       \
	XX(fcntl)       \

	void hook_init()
	{
		static bool is_inited = false;
		if (is_inited)
		{
			return;
		}
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
		HOOK_FUN(XX);
#undef XX
	}

	static uint64_t s_connect_timeout = -1;
	struct _HookIniter
	{
		_HookIniter()
		{
			hook_init();
			s_connect_timeout = g_tcp_connect_timeout->getValue();
		
			g_tcp_connect_timeout->addListener([](const int& old_val, const int& new_val){
				ROUTN_LOG_INFO(g_logger) << "tcp connect timeout changed from "
									 	 << old_val << " to " << new_val;
				s_connect_timeout = new_val;
			});
		}
	};

	static _HookIniter s_hook_initer;

	bool is_hook_enable()
	{
		return t_hook_enable;
	}

	void set_hook_enable(bool flag)
	{
		t_hook_enable = flag;
	}

}

struct timer_info
{
	int cancelled = 0;
};

template <typename OriginFun, typename... ARGS>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_name,
					 uint32_t event, int timeout_so, ARGS &&...args)
{
	if (!Routn::t_hook_enable)
	{
		return fun(fd, std::forward<ARGS>(args)...);
	}

	Routn::FdCtx::ptr ctx = Routn::FdMgr::GetInstance()->get(fd);
	if (!ctx)
	{
		return fun(fd, std::forward<ARGS>(args)...);
	}

	if (ctx->isClose())
	{
		errno = EBADF;
		return -1;
	}

	if (!ctx->isSocket() || ctx->getUserNonblock())
	{
		return fun(fd, std::forward<ARGS>(args)...);
	}

	uint64_t timeout = ctx->getTimeout(timeout_so);
	std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
	ssize_t n = fun(fd, std::forward<ARGS>(args)...);
	while (n == -1 && errno == EINTR)
	{
		n = fun(fd, std::forward<ARGS>(args)...);
	}
	if (n == -1 && errno == EAGAIN)
	{
		Routn::IOManager *iom = Routn::IOManager::GetThis();
		Routn::Timer::ptr timer;
		std::weak_ptr<timer_info> winfo(tinfo);

		if (timeout != (uint64_t)-1)
		{
			timer = iom->addConditionTimer(
				timeout, [winfo, fd, iom, event]()
				{
					auto t = winfo.lock();
					if (!t || t->cancelled)
					{
						return;
					}
					t->cancelled = ETIMEDOUT;
					iom->cancelEvent(fd, (Routn::IOManager::Event)(event));
				},
				winfo);
		}

		int ret = iom->addEvent(fd, (Routn::IOManager::Event)(event));
		if (ROUTN_UNLIKELY(ret))
		{
			ROUTN_LOG_ERROR(Routn::g_logger) << hook_name << "addEvent("
											 << fd << ", " << event << ")";
			if (timer)
			{
				timer->cancel();
			}
			return -1;
		}
		else
		{
			Routn::Fiber::YieldToHold();
			if (timer)
			{
				timer->cancel();
			}
			if (tinfo->cancelled)
			{
				errno = tinfo->cancelled;
				return -1;
			}

			goto retry;
		}
	}
	return n;
}

extern "C"
{
#define XX(name) name##_fun name##_f = nullptr;
	HOOK_FUN(XX);
#undef XX

	unsigned int sleep(unsigned int seconds)
	{
		if (!Routn::t_hook_enable)
		{
			return sleep_f(seconds);
		}

		Routn::Fiber::ptr fiber = Routn::Fiber::GetThis();
		Routn::IOManager *iom = Routn::IOManager::GetThis();
		iom->addTimer(seconds * 1000, std::bind((void (Routn::Schedular::*)(Routn::Fiber::ptr, int thread)) & Routn::IOManager::schedule, iom, fiber, -1));
		//iom->addTimer(seconds * 1000, [iom, fiber](){
		//	iom->schedule(fiber);
		//});
		Routn::Fiber::YieldToHold();
		return 0;
	}

	int usleep(useconds_t usec)
	{
		if (!Routn::t_hook_enable)
		{
			return usleep_f(usec);
		}
		Routn::Fiber::ptr fiber = Routn::Fiber::GetThis();
		Routn::IOManager *iom = Routn::IOManager::GetThis();
		iom->addTimer(usec / 1000, std::bind((void (Routn::Schedular::*)(Routn::Fiber::ptr, int thread)) & Routn::IOManager::schedule, iom, fiber, -1));
		//iom->addTimer(usec / 1000, [iom, fiber](){
		//	iom->schedule(fiber);
		//});
		Routn::Fiber::YieldToHold();
		return 0;
	}

	int nanosleep(const struct timespec *req, struct timespec *rem)
	{
		if (!Routn::t_hook_enable)
		{
			return nanosleep_f(req, rem);
		}
		int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;

		Routn::Fiber::ptr fiber = Routn::Fiber::GetThis();
		Routn::IOManager *iom = Routn::IOManager::GetThis();
		iom->addTimer(timeout_ms, std::bind((void (Routn::Schedular::*)(Routn::Fiber::ptr, int thread)) & Routn::IOManager::schedule, iom, fiber, -1));
		//iom->addTimer(timeout_ms, [iom, fiber](){
		//	iom->schedule(fiber);
		//});
		Routn::Fiber::YieldToHold();
		return 0;
	}

	int socket(int domain, int type, int protocol)
	{
		if (!Routn::t_hook_enable)
		{
			return socket_f(domain, type, protocol);
		}
		int fd = socket_f(domain, type, protocol);
		if (fd == -1)
		{
			return fd;
		}
		Routn::FdMgr::GetInstance()->get(fd, true);
		return fd;
	}

	int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout){
		if(!Routn::t_hook_enable){
			return connect_f(fd, addr, addrlen);
		}
		Routn::FdCtx::ptr ctx = Routn::FdMgr::GetInstance()->get(fd);
		if(!ctx || ctx->isClose()){
			errno = EBADF;
			return -1;
		}

		if(!ctx->isSocket()){
			return connect_f(fd, addr, addrlen);
		}

		if(ctx->getUserNonblock()){
			return connect_f(fd, addr, addrlen);
		}

		int n = connect_f(fd, addr, addrlen);
		if(n == 0){
			return 0;
		}else if(n != -1 || errno != EINPROGRESS){
			return n;
		}

		Routn::IOManager* iom = Routn::IOManager::GetThis();
		Routn::Timer::ptr timer;
		std::shared_ptr<timer_info> tinfo(new timer_info);
		std::weak_ptr<timer_info> winfo(tinfo);

		if(timeout != (uint64_t)-1){
			timer = iom->addConditionTimer(timeout, [winfo, fd, iom](){
				auto t = winfo.lock();
				if(!t || t->cancelled){
					return ;
				}
				t->cancelled = ETIMEDOUT;
				iom->cancelEvent(fd, Routn::IOManager::WRITE);
			}, winfo);
		}

		int ret = iom->addEvent(fd, Routn::IOManager::WRITE);
		if(ret == 0){
			Routn::Fiber::YieldToHold();
			if(timer){
				timer->cancel();
			}
			if(tinfo->cancelled){
				errno = tinfo->cancelled;
				return -1;
			}
		}else{
			if(timer){
				timer->cancel();
			}
			ROUTN_LOG_ERROR(Routn::g_logger) << "connect addEvent(" << fd << ", WRITE) error";
		}
		int error = 0;
		socklen_t len = sizeof(int);
		if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)){
			return -1;
		}
		if(!error){
			return 0;
		}else{
			errno = error;
			return -1;
		}
	}

	int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
	{
		return connect_with_timeout(sockfd, addr, addrlen, Routn::s_connect_timeout);
	}

	int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
	{
		int fd = do_io(s, accept_f, "accept", Routn::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
		if (fd >= 0)
		{
			Routn::FdMgr::GetInstance()->get(fd, true);
		}
		return fd;
	}

	ssize_t read(int fd, void *buf, size_t count)
	{
		return do_io(fd, read_f, "read", Routn::IOManager::READ, SO_RCVTIMEO, buf, count);
	}

	ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
	{
		return do_io(fd, readv_f, "readv", Routn::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
	}

	ssize_t recv(int sockfd, void *buf, size_t len, int flags)
	{
		return do_io(sockfd, recv_f, "recv", Routn::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
	}

	ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
	{
		return do_io(sockfd, recvfrom_f, "recvfrom", Routn::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
	}

	ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
	{
		return do_io(sockfd, recvmsg_f, "recvmsg", Routn::IOManager::READ, SO_RCVTIMEO, msg, flags);
	}

	ssize_t write(int fd, const void *buf, size_t count)
	{
		return do_io(fd, write_f, "write", Routn::IOManager::WRITE, SO_SNDTIMEO, buf, count);
	}

	ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
	{
		return do_io(fd, writev_f, "writev", Routn::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
	}

	ssize_t send(int sockfd, const void *buf, size_t len, int flags)
	{
		return do_io(sockfd, send_f, "send", Routn::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
	}

	ssize_t sendto(int sockfd, const void *buf, size_t len, int flag, const struct sockaddr *dest_addr, socklen_t addrlen)
	{
		return do_io(sockfd, sendto_f, "sendto", Routn::IOManager::WRITE, SO_SNDTIMEO, buf, len, flag, dest_addr, addrlen);
	}

	ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
	{
		return do_io(sockfd, sendmsg_f, "sendmsg", Routn::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
	}

	int close(int fd)
	{
		if (!Routn::t_hook_enable)
		{
			return close_f(fd);
		}

		Routn::FdCtx::ptr ctx = Routn::FdMgr::GetInstance()->get(fd);
		if (ctx)
		{
			auto iom = Routn::IOManager::GetThis();
			if (iom)
			{
				iom->cancelAll(fd);
			}
			Routn::FdMgr::GetInstance()->del(fd);
		}
		return close_f(fd);
	}

	int fcntl(int fd, int cmd, ... /* arg */)
	{
		va_list va;
		va_start(va, cmd);
		switch (cmd)
		{
		case F_SETFL:
		{
			int arg = va_arg(va, int);
			va_end(va);
			Routn::FdCtx::ptr ctx = Routn::FdMgr::GetInstance()->get(fd);
			if (!ctx || ctx->isClose() || !ctx->isSocket())
			{
				return fcntl_f(fd, cmd, arg);
			}
			ctx->setUserNonblock(arg & O_NONBLOCK);
			if (ctx->getSysNonblock())
			{
				arg |= O_NONBLOCK;
			}
			else
			{
				arg &= ~O_NONBLOCK;
			}
			return fcntl_f(fd, cmd, arg);
		}
		break;
		case F_GETFL:
		{
			va_end(va);
			int arg = fcntl_f(fd, cmd);
			Routn::FdCtx::ptr ctx = Routn::FdMgr::GetInstance()->get(fd);
			if (!ctx || ctx->isClose() || ctx->isSocket())
			{
				return arg;
			}
			if (ctx->getUserNonblock())
			{
				return arg | O_NONBLOCK;
			}
			else
			{
				return arg & ~O_NONBLOCK;
			}
		}
		break;
		case F_DUPFD:
		case F_DUPFD_CLOEXEC:
		case F_SETFD:
		case F_SETSIG:
		case F_SETLEASE:
		case F_NOTIFY:
		case F_SETPIPE_SZ:
		{
			int arg = va_arg(va, int);
			va_end(va);
			int ret = fcntl_f(fd, cmd, arg);
			return ret;
		}
		break;
		case F_GETFD:
		case F_GETOWN:
		case F_GETSIG:
		case F_GETLEASE:
		case F_GETPIPE_SZ:
		{
			va_end(va);
			return fcntl_f(fd, cmd);
		}
		break;
		case F_SETLK:
		case F_SETLKW:
		case F_GETLK:
		{
			struct flock *arg = va_arg(va, struct flock *);
			va_end(va);
			return fcntl_f(fd, cmd, arg);
		}
		break;
		case F_GETOWN_EX:
		case F_SETOWN_EX:
		{
			struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
			va_end(va);
			return fcntl_f(fd, cmd, arg);
		}
		break;
		default:
			va_end(va);
			return fcntl_f(fd, cmd);
		}
	}

	int ioctl(int d, unsigned long int request, ...)
	{
		va_list va;
		va_start(va, request);
		void* arg = va_arg(va, void*);
		va_end(va);

		if(FIONBIO == request){
			bool user_nonblock = !!*(int*)arg;
			Routn::FdCtx::ptr ctx = Routn::FdMgr::GetInstance()->get(d);
			if(!ctx || ctx->isClose() || !ctx->isSocket()){
				return ioctl_f(d, request, arg);
			}
			ctx->setUserNonblock(user_nonblock);
		}
		return ioctl_f(d, request, arg);
	}

	int getsockopt(int sockfd, int level, int optname,
				   void *optval, socklen_t *optlen)
	{
		return getsockopt_f(sockfd, level, optname, optval, optlen);
	}

	int setsockopt(int sockfd, int level, int optname,
				   const void *optval, socklen_t optlen)
	{
		if(!Routn::t_hook_enable){
			return setsockopt_f(sockfd, level, optname, optval, optlen);
		}
		if(level == SOL_SOCKET){
			if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO || optname == SO_SNDTIMEO_NEW || optname == SO_RCVTIMEO_NEW){
				Routn::FdCtx::ptr ctx = Routn::FdMgr::GetInstance()->get(sockfd);
				if(ctx){
					const timeval* v = (const timeval*)optval;
					ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
				}
			}
		}
		return setsockopt_f(sockfd, level, optname, optval, optlen);
	}
}

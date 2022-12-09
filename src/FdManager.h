/*************************************************************************
	> File Name: FdManager.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月03日 星期四 19时07分52秒
 ************************************************************************/

#ifndef _FDMANAGER_H
#define _FDMANAGER_H

#include <memory>
#include <vector>
#include <stdint.h>
#include <sys/stat.h>
#include <bits/types.h>
#include "Singleton.h"
#include "Thread.h"
#include "Hook.h"

namespace Routn
{
    class FdCtx : public std::enable_shared_from_this<FdCtx>
    {
    public:
        using ptr = std::shared_ptr<FdCtx>;
        FdCtx(int fd);
        ~FdCtx();

        bool init();
        bool isInit() const { return _isInit; }
        bool isSocket() const { return _isSocket; }
        bool isClose() const { return _isClosed; }

        void setUserNonblock(bool v) { _userNonblock = v; }
        bool getUserNonblock() const { return _userNonblock; }

        void setSysNonblock(bool v) { _sysNonblock = v; }
        bool getSysNonblock() const { return _sysNonblock; }

        void setTimeout(int type, uint64_t v);
        uint64_t getTimeout(int type);

    private:
        bool _isInit : 1;
        bool _isSocket : 1;
        bool _sysNonblock : 1;
        bool _userNonblock : 1;
        bool _isClosed : 1;
        int _fd;

        uint64_t _recvTimeout;
        uint64_t _sendTimeout;
    };

    class FdManager
    {
    public:
        using RWMutexType = RW_Mutex;

        FdCtx::ptr get(int fd, bool auto_create = false);
        void del(int fd);
        FdManager();

    private:
        RWMutexType _mutex;
        std::vector<FdCtx::ptr> _datas;
    };

    using FdMgr = Singleton<FdManager>;
}
#endif

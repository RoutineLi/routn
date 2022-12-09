/*************************************************************************
	> File Name: Noncopyable.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月06日 星期日 14时37分23秒
 ************************************************************************/

#ifndef _NONCOPYABLE_H
#define _NONCOPYABLE_H

namespace Routn{
    
    class Noncopyable{
    public:
        Noncopyable() = default;
        ~Noncopyable() = default;
        Noncopyable(const Noncopyable&) = delete;
        Noncopyable& operator=(const Noncopyable&) = delete;
    };
}


#endif

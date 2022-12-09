/*************************************************************************
	> File Name: singleton.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月26日 星期三 14时56分45秒
 ************************************************************************/

#ifndef _SINGLETON_H
#define _SINGLETON_H

#include <memory>

namespace Routn{
    
    template<class T, class X = void, int N = 0>
    class Singleton {
    public:
        static T* GetInstance(){
            static T v;
            return &v;
        }
    };

    template<class T, class X = void, int N = 0>
    class SingletonPtr{
    public:
        static std::shared_ptr<T> GetInstance(){
            static std::shared_ptr<T> v(new T);
            return v;
        }
    };
};

#endif

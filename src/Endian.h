/*************************************************************************
	> File Name: Endian.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月05日 星期六 15时17分46秒
 ************************************************************************/

#ifndef __ENDIAN_H_
#define __ENDIAN_H_

#define ROUTN_LITTLE_ENDIAN 1
#define ROUTN_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>
#include <type_traits>

/**
 * 主机大小端转换库
 **/

namespace Routn
{

	template <class T>
	typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
	byteswap(T value)
	{
		return (T)bswap_64((uint64_t)value);
	}

	template <class T>
	typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
	byteswap(T value)
	{
		return (T)bswap_32((uint32_t)value);
	}

	template <class T>
	typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
	byteswap(T value)
	{
		return (T)bswap_16((uint16_t)value);
	}

#if BYTE_ORDER == BIG_ENDIAN
#define ROUTN_BYTE_ORDER ROUTN_BIG_ENDIAN
#else
#define ROUTN_BYTE_ORDER ROUTN_LITTLE_ENDIAN
#endif

#if ROUTN_BYTE_ORDER == ROUTN_BIG_ENDIAN
	template <class T>
	T byteswapOnLittleEndian(T t)
	{
		return t;
	}

	template <class T>
	T byteswapOnBigEndian(T t)
	{
		return byteswap(t);
	}

#else
	template <class T>
	T byteswapOnLittleEndian(T t)
	{
		return byteswap(t);
	}

	template <class T>
	T byteswapOnBigEndian(T t)
	{
		return t;
	}
#endif
}

#endif

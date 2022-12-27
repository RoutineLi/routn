/*************************************************************************
	> File Name: chat_protocol.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月18日 星期日 14时54分45秒
 ************************************************************************/

#ifndef _CHAT_PROTOCOL_H
#define _CHAT_PROTOCOL_H

#include <string>
#include <memory>
#include <map>

namespace Chat{

class ChatMessage{
public:
	using ptr = std::shared_ptr<ChatMessage>;
	static ChatMessage::ptr Create(const std::string& v);
	ChatMessage();
	std::string get(const std::string& name);
	void set(const std::string& name, const std::string& val);
	std::string toString() const;
private:
	std::map<std::string, std::string> _datas;
};

}


#endif

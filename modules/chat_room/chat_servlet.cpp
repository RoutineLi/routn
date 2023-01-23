/*************************************************************************
	> File Name: chat_servlet.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年12月16日 星期五 19时57分40秒
 ************************************************************************/

#include "chat_servlet.h"
#include "chat_protocol.h"


#include <iostream>
#include <fstream>

static Routn::Logger::ptr g_logger = ROUTN_LOG_NAME("system");

namespace Routn{
namespace Http{
	ResourceServlet::ResourceServlet(const std::string& path)
		: Servlet("ResourceServlet")
		, _path(path){

	}

	int32_t ResourceServlet::handle(Routn::Http::HttpRequest::ptr request, 
							Routn::Http::HttpResponse::ptr response,
							Routn::SocketStream::ptr session){
		std::string path = _path + request->getPath();
		//std::cout << path << std::endl;

		if(path.find("..") != path.npos){
			response->setBody("invalid file");
			response->setStatus(Routn::Http::HttpStatus::NOT_FOUND);
			return 0;
		}
		std::ifstream ifs(path);
		if(!ifs){
			response->setBody("invalid file");
			response->setStatus(Routn::Http::HttpStatus::NOT_FOUND);
			return 0;
		}

		std::stringstream ss;
		std::string line;

		while(std::getline(ifs, line)){
			ss << line << std::endl;
		}

		response->setBody(ss.str());
		return 0;
	}
}	
}

Routn::RW_Mutex s_mutex;
std::map<std::string, Routn::Http::WSSession::ptr> s_sessions;

int32_t SendMessage(Routn::Http::WSSession::ptr session, Chat::ChatMessage::ptr msg){
	return session->sendMessage(msg->toString()) > 0 ? 0 : 1;
}

bool session_exists(const std::string& id){
	ROUTN_LOG_INFO(g_logger) << "session id= " << id;
	Routn::RW_Mutex::ReadLock lock(s_mutex);
	auto it = s_sessions.find(id);
	return it != s_sessions.end();
}

void session_add(const std::string& id, Routn::Http::WSSession::ptr session){
	ROUTN_LOG_INFO(g_logger) << "session id= " << id;
	Routn::RW_Mutex::WriteLock lock(s_mutex);
	s_sessions[id] = session;
}

void session_del(const std::string& id){
	ROUTN_LOG_INFO(g_logger) << "session id= " << id;
	Routn::RW_Mutex::WriteLock lock(s_mutex);
	s_sessions.erase(id);
}

void session_notify(Chat::ChatMessage::ptr msg, Routn::Http::WSSession::ptr session = nullptr){
	Routn::RW_Mutex::ReadLock lock(s_mutex);
	auto sessions = s_sessions;
	lock.unlock();

	for(auto &i : sessions){
		if(i.second == session){
			continue;
		}
		SendMessage(i.second, msg);
	}
}

ChatServlet::ChatServlet()
	: WSServlet("ws-chat/routn1.1") {

}

int32_t ChatServlet::onConnect(Routn::Http::HttpRequest::ptr header
				, Routn::Http::WSSession::ptr session){
	return 0;
}

int32_t ChatServlet::onClose(Routn::Http::HttpRequest::ptr header
				, Routn::Http::WSSession::ptr session){
	auto id = header->getHeader("$id");
	if(!id.empty()){
		session_del(id);
		Chat::ChatMessage::ptr nty(new Chat::ChatMessage);
		nty->set("type", "user_leave");
		nty->set("time", Routn::TimerToString());
		nty->set("name", id);
		session_notify(nty);
	}
	return 0;
}

int32_t ChatServlet::handle(Routn::Http::HttpRequest::ptr header
				, Routn::Http::WSFrameMessage::ptr msg
				, Routn::Http::WSSession::ptr session){
	ROUTN_LOG_INFO(g_logger) << "handle: " << header->toString() << "\nopcode = " << msg->getOpcode()
		<< " data = " << msg->getData();
	
	auto cmsg = Chat::ChatMessage::Create(msg->getData());
	auto id = header->getHeader("$id");
	ROUTN_LOG_INFO(g_logger) << "session id= " << id;
	if(!msg){
		if(!id.empty()){
			Routn::RW_Mutex::WriteLock lock(s_mutex);
			s_sessions.erase(id);
		}
		return 1;
	}

	Chat::ChatMessage::ptr rsp(new Chat::ChatMessage);

	if(cmsg->get("type") == "login_request"){
		rsp->set("type", "login_response");
		auto name = cmsg->get("name");
		if(name.empty()){
			rsp->set("result", "400");
			rsp->set("msg", "name is null");
			return SendMessage(session, rsp);
		}
		if(!id.empty()){
			rsp->set("result", "401");
			rsp->set("msg", "logined");
			return SendMessage(session, rsp);
		}
		if(session_exists(name)){
			rsp->set("result", "402");
			rsp->set("msg", "name exists");
			return SendMessage(session, rsp);
		}
		id = name;
		header->setHeader("$id", id);
		rsp->set("result", "200");
		rsp->set("msg", "ok");
		session_add(id, session);

		Chat::ChatMessage::ptr nty(new Chat::ChatMessage);
		nty->set("type", "user_enter");
		nty->set("time", Routn::TimerToString());
		nty->set("name", name);
		session_notify(nty, session);

		return SendMessage(session, rsp);


	}else if(cmsg->get("type") == "send_request"){
		rsp->set("type", "send_response");
		auto m = cmsg->get("msg");
		if(m.empty()){
			rsp->set("result", "500");
			rsp->set("msg", "msg is null");
			return SendMessage(session, rsp);
		}
		if(id.empty()){
			rsp->set("result", "501");
			rsp->set("msg", "not login");
			return SendMessage(session, rsp);
		}

		rsp->set("result", "200");
		rsp->set("msg", "ok");

		Chat::ChatMessage::ptr nty(new Chat::ChatMessage);
		nty->set("type", "msg");
		nty->set("time", Routn::TimerToString());
		nty->set("name", id);
		nty->set("msg", m);
		session_notify(nty, nullptr);

		return SendMessage(session, rsp);
	}

	return 0;
}

/*************************************************************************
	> File Name: test_http_parser.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月18日 星期五 16时25分40秒
 ************************************************************************/

#include "../src/http/Parser.h"
#include "../src/Routn.h"

Routn::Logger::ptr g_logger = ROUTN_LOG_ROOT();

const char test_request_data[] = "POST / HTTP/1.1\r\n"
								"Host: www.baidu.com\r\n"
								"Content-Length: 10\r\n\r\n"
								"1234567890";

void test1(){
	Routn::Http::HttpRequestParser parser;
	std::string temp = test_request_data;
	auto s = parser.execute(&temp[0], temp.size());
	ROUTN_LOG_INFO(g_logger) << "execute ret = " << s
		<< " has_error = " << parser.hasError()
		<< " is_finished = " << parser.isFinished()
		<< " total = " << temp.size()
		<< " content-length = " << parser.getContentLength();
	temp.resize(temp.size() - s);
	ROUTN_LOG_INFO(g_logger) << parser.getData()->toString();
	ROUTN_LOG_INFO(g_logger) << temp;
}

const char test_response_data[] = "HTTP/1.1 200 OK\r\n"
        "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
        "Server: Apache\r\n"
        "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
        "ETag: \"51-47cf7e6ee8400\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 81\r\n"
        "Cache-Control: max-age=86400\r\n"
        "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
        "</html>\r\n";
void test2(){
	Routn::Http::HttpResponseParser parser;
	std::string temp = test_response_data;
	size_t s = parser.execute(&temp[0], temp.size(), true);
	ROUTN_LOG_ERROR(g_logger) << "execute ret = " << s
		<< " has error = " << parser.hasError()
		<< " is finished = " << parser.isFinished()
		<< " total = " << temp.size()
		<< " content_length";
	temp.resize(temp.size() - s);
	ROUTN_LOG_INFO(g_logger) << parser.getData()->toString();
	ROUTN_LOG_INFO(g_logger) << temp;
}

int main(int argc, char** argv){
	test2();
	return 0;
}




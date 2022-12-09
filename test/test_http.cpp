/*************************************************************************
	> File Name: test_http.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月08日 星期二 17时53分41秒
 ************************************************************************/

#include "../src/Routn.h"
#include "../src/http/HTTP.h"

void test(){
	Routn::Http::HttpRequest::ptr req(new Routn::Http::HttpRequest);
	req->setHeader("host", "www.baidu.com");
	req->setBody("hello i am Routn");

	req->dump(std::cout) << std::endl;
}

void test2(){
	Routn::Http::HttpResponse::ptr rsp(new Routn::Http::HttpResponse);
	rsp->setHeader("X-X", "routn");
	rsp->setBody("hello Routn");
	rsp->setStatus((Routn::Http::HttpStatus)400);
	rsp->setClose(false);
	rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv){
	test();
	return 0;
}


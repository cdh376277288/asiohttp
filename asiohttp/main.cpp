#include "http_client.h"


int main()
{
	//std::string str("http://yunhq.sse.com.cn:32041/v1/sh1/snap/204001?callback=jQuery_test&select=name%2Clast%2Cchg_rate%2Cchange%2Camount%2Cvolume%2Copen%2Cprev_close%2Cask%2Cbid%2Chigh%2Clow%2Ctradephase");
	//str = cdh::get(str);
	//std::cout << str.c_str() << std::endl;

	//str = "http://service.winic.org:8009/sys_port/gateway/[id=13695800360&pwd=13645411460&to=13695800360&content=infomation&time=]";
	//str = cdh::post(str);
	//std::cout << str.c_str() << std::endl;

	std::string str1("http://yunhq.sse.com.cn:32041/v1/sh1/snap/204001?callback=jQuery_test&select=name%2Clast%2Cchg_rate%2Cchange%2Camount%2Cvolume%2Copen%2Cprev_close%2Cask%2Cbid%2Chigh%2Clow%2Ctradephase");
	std::string str2("http://service.winic.org:8009/sys_port/gateway/[id=13695800360&pwd=13645411460&to=13695800360&content=infomation&time=]");
	std::string resp;

	std::string str3("https://api.krlxinfukang.com/users/account/login[{\"username\":\"18702302947\",\"password\" : \"123456\",\"loginType\" : \"0\",\"type\" : \"2\"}]");
	boost::asio::io_service io;
	cdh::HttpClient c(io);
	//c.get(str1);
	c.post(str2);
	io.run();
	resp = c.getResponse();
	std::cout << resp << std::endl;

	return 0;
}
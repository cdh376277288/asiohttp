// AsyncHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include "AsyncHttpClient.hpp"
#include "AsyncHttpClient2.hpp"
#include "AsyncHttpServer.hpp"
#include <iostream>

using namespace Http;

void test_callback(AsyncClient::Ptr client) {
	if (!client->IsSucceed()) {
		std::cout << client->GetErrorMessage() << std::endl;
		return;
	}
	std::cout << client->GetResponse() << std::endl;
}

void test_callback2(AsyncClient2::Ptr client) {
	if (!client->IsSucceed()) {
		std::cout << client->GetErrorMessage() << std::endl;
		return;
	}
	std::cout << client->GetResponse() << std::endl;
}

void test_server(AsyncServerConnection::Ptr connection) {
	connection->WriteResponse("OK");
}

int test()
{
	boost::asio::io_service service;

 	//AsyncClient::Ptr client = boost::make_shared<AsyncClient>(service);
 	//assert(client->Parse("http://www.cnblogs.com:999/haha?name1=100&name2=10234"));
 	//assert(client->Parse("http://www.cnblogs.com/?name1=100&name2=10234"));
 	//assert(client->Parse("http://www.cnblogs.com"));
 	//assert(client->Parse("http://www.cnblogs.com:80/haha"));
 	//assert(client->Parse("http://www.cnblogs.com:80"));
 
 	//client->Get(&test_callback);

	//AsyncServer::Ptr server = boost::make_shared<AsyncServer>(boost::ref(service), &test_server);
	//server->Startup("0.0.0.0", 80);

 	AsyncClient2::Ptr client = boost::make_shared<AsyncClient2>(service);
 	//assert(client->Parse("http://www.cnblogs.com:999/haha?name1=100&name2=10234"));
 	//assert(client->Parse("http://www.cnblogs.com/?name1=100&name2=10234"));
 	//assert(client->Parse("http://www.cnblogs.com"));
 	//assert(client->Parse("http://www.cnblogs.com:80"));
 	assert(client->Parse("http://192.168.1.128:8660/users/consulation/im/updateKrlIm"));
	//assert(client->Get());
	//CString tempstr = "{'name':'张三','age':'18','sex':'男','userid':3}";
	client->Post("{'userid':'2','type':'3'}", &test_callback2);
 	//client->Get(&test_callback2);

	service.run();
	std::cout << "123123" << std::endl;
	std::cout << "123123" << std::endl;
	std::cout << "123123" << std::endl;
	std::cout << "123123" << std::endl;
   // return 0;
}
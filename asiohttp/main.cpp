//
// async_client.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <queue>
#include <unordered_map>
#include <memory>
#include <functional>
#include <boost/asio/spawn.hpp>   
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>

using HttpResponseCallBack = std::function<void(int, std::string response)>;

using boost::asio::ip::tcp;



class HttpRequest;
//TODO cache http ip
//TODO auto thread-load-balance
//TODO process timeout
//单线程httpclient 处理能力在 1000条每秒左右 足以 一台机器 fd 50个进程差不多达到了上限了
class HttpClient
{
public:
	void AsyncRun();
	void Get(const std::string& url, const std::string& uri, const HttpResponseCallBack& cb);
private:

	//process timeout for http
	void LoopTimer(boost::asio::yield_context yield);
private:
	friend class HttpRequest;
	void HttpDone(HttpRequest* req)
	{
		auto it = _processing_requestes.find(req);
		if (it != _processing_requestes.end())
		{
			_processing_requestes.erase(it);
		}
	}
	boost::asio::io_service _io;
	std::unique_ptr<std::thread> _t;
	std::unordered_map<HttpRequest*, std::shared_ptr<HttpRequest>> _processing_requestes;//for process timeout
};


class HttpRequest :public std::enable_shared_from_this<HttpRequest>
{
private:
	std::string server;
public:
	HttpRequest(HttpClient* client, boost::asio::io_service& io_service,
		const std::string& server, const std::string& uri, HttpResponseCallBack cb)
		: resolver_(io_service),
		socket_(io_service),
		cb(cb),
		server(server),
		client(client)
	{
		time_start = time(nullptr);
		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		std::ostream request_stream(&request_);
		request_stream << "GET " << uri << " HTTP/1.0\r\n";
		request_stream << "Host: " << server << "\r\n";
		request_stream << "Accept: */*\r\n";
		request_stream << "Connection: close\r\n\r\n";
	}
	void Go()
	{
		// Start an asynchronous resolve to translate the server and service names
		// into a list of endpoints.
		tcp::resolver::query query(server, "http");
		resolver_.async_resolve(query,
			boost::bind(&HttpRequest::handle_resolve, this->shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::iterator));
	}
private:
	void handle_resolve(const boost::system::error_code& err,
		tcp::resolver::iterator endpoint_iterator)
	{
		if (!err)
		{
			// Attempt a connection to each endpoint in the list until we
			// successfully establish a connection.
			boost::asio::async_connect(socket_, endpoint_iterator,
				boost::bind(&HttpRequest::handle_connect, this->shared_from_this(),
					boost::asio::placeholders::error));
		}
		else
		{
			CallCB(0);
			if (this->client)
			{
				this->client->HttpDone(this);
			}
		}
	}

	void handle_connect(const boost::system::error_code& err)
	{
		if (!err)
		{
			// The connection was successful. Send the request.
			boost::asio::async_write(socket_, request_,
				boost::bind(&HttpRequest::handle_write_request, this->shared_from_this(),
					boost::asio::placeholders::error));
		}
		else
		{
			CallCB(0);
			if (this->client)
			{
				this->client->HttpDone(this);
			}
			//std::cout << "Error: " << err.message() << "\n";
		}
	}

	void handle_write_request(const boost::system::error_code& err)
	{
		if (!err)
		{
			// Read the response status line. The response_ streambuf will
			// automatically grow to accommodate the entire line. The growth may be
			// limited by passing a maximum size to the streambuf constructor.
			boost::asio::async_read_until(socket_, response_, "\r\n",
				boost::bind(&HttpRequest::handle_read_status_line, this->shared_from_this(),
					boost::asio::placeholders::error));
		}
		else
		{
			CallCB(0);
			if (this->client)
			{
				this->client->HttpDone(this);
			}
			//std::cout << "Error: " << err.message() << "\n";
		}
	}

	void handle_read_status_line(const boost::system::error_code& err)
	{
		if (!err)
		{
			// Check that response is OK.
			std::istream response_stream(&response_);
			std::string http_version;
			response_stream >> http_version;
			unsigned int status_code;
			response_stream >> status_code;
			std::string status_message;
			std::getline(response_stream, status_message);
			if (!response_stream || http_version.substr(0, 5) != "HTTP/")
			{
				//std::cout << "Invalid response\n";
				CallCB(0);
				if (this->client)
				{
					this->client->HttpDone(this);
				}
				return;
			}
			if (status_code != 200)
			{
				//	std::cout << "Response returned with status code ";
				//	std::cout << status_code << "\n";
				CallCB(status_code);
				if (this->client)
				{
					this->client->HttpDone(this);
				}
				return;
			}

			// Read the response headers, which are terminated by a blank line.
			boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
				boost::bind(&HttpRequest::handle_read_headers, this->shared_from_this(),
					boost::asio::placeholders::error));
		}
		else
		{
			CallCB(0);
			if (this->client)
			{
				this->client->HttpDone(this);
			}
			//std::cout << "Error: " << err << "\n";
		}
	}

	void handle_read_headers(const boost::system::error_code& err)
	{
		if (!err)
		{
			// Process the response headers.
			//		std::istream response_stream(&response_);
			//	std::string header;
			/*	while (std::getline(response_stream, header) && header != "\r")
					std::cout << header << "\n";
					std::cout << "\n";
					*/
					// Write whatever content we already have to output.
					//	if (response_.size() > 0)
			{
				//	std::cout << &response_;
			}
			// Start reading remaining data until EOF.
			boost::asio::async_read(socket_, response_,
				boost::asio::transfer_at_least(1),
				boost::bind(&HttpRequest::handle_read_content, this->shared_from_this(),
					boost::asio::placeholders::error));
		}
		else
		{
			CallCB(0);
			if (this->client)
			{
				this->client->HttpDone(this);
			}
			//	std::cout << "Error: " << err << "\n";
		}
	}

	void handle_read_content(const boost::system::error_code& err)
	{
		if (!err)
		{
			// Write all of the data that has been read so far.
			//	std::cout << &response_;
			// Continue reading remaining data until EOF.
			boost::asio::async_read(socket_, response_,
				boost::asio::transfer_at_least(1),
				boost::bind(&HttpRequest::handle_read_content, this->shared_from_this(),
					boost::asio::placeholders::error));
		}
		else if (err != boost::asio::error::eof)
		{
			CallCB(0);
			if (this->client)
			{
				this->client->HttpDone(this);
			}
			//	std::cout << "Error: " << err << "\n";
		}
		else
		{
			boost::asio::streambuf::const_buffers_type cbt = response_.data();
			std::string str(boost::asio::buffers_begin(cbt), boost::asio::buffers_end(cbt));

			auto pos = str.find("Content-Length: ");
			std::vector<char> buf;
			int content_len = 0;
			if (pos != std::string::npos)
			{
				for (int i = pos + 16; i < str.size(); i++)
				{
					char c = str[i];
					if (c >= '0' && c <= '9')
					{
						buf.push_back(c - '0');
					}
					else
					{
						break;
					}
				}
				if (buf.size() > 0)
				{
					for (int i = buf.size() - 1, len = 1; i >= 0; len *= 10, --i)
					{
						content_len += buf[i] * len;
					}
				}
				std::string sub = str.substr(str.size() - content_len);
				CallCB(200, sub);
				if (this->client)
				{
					this->client->HttpDone(this);
				}
			}
			else
			{
				//raw get
				CallCB(200, str);
				if (this->client)
				{
					this->client->HttpDone(this);
				}
			}
		}
	}
	inline void CallCB(int code, std::string response = std::string())
	{
		TryCloseSocket();
		if (has_callback)return;
		has_callback = true;
		cb(code, response);
	}

	inline void TryCloseSocket()
	{
		if (socket_open == false)return;
		socket_open = false;
		try
		{
			boost::system::error_code ec;
			socket_.close(ec);
		}
		catch (std::exception e)
		{

		}
	}
	friend class HttpClient;
	bool socket_open = true;
	bool has_callback = false;
	int time_start = 0;
	tcp::resolver resolver_;
	tcp::socket socket_;
	boost::asio::streambuf request_;
	boost::asio::streambuf response_;
	HttpResponseCallBack cb;
	HttpClient* client = nullptr;
public:
	virtual ~HttpRequest()
	{

	}
};


void HttpClient::LoopTimer(boost::asio::yield_context yield)
{
	boost::asio::steady_timer timer(_io);
	while (true)
	{
		timer.expires_from_now(std::chrono::seconds(2));
		boost::system::error_code ec;
		timer.async_wait(yield[ec]);

		if (_processing_requestes.size() == 0)continue;

		int now = time(nullptr);
		std::list<HttpRequest*> expire;
		for (auto it : _processing_requestes)
		{
			if (it.second)
			{
				if (now - it.second->time_start > 5)
				{
					it.second->CallCB(0);
					expire.push_back(it.first);
				}
			}
		}
		for (auto it = expire.begin(); it != expire.end(); ++it)
		{
			_processing_requestes.erase(*it);
		}
	}
}

void HttpClient::AsyncRun()
{
	if (_t == nullptr)
	{
		_t.reset(new std::thread([this]()
			{
				boost::asio::io_service::work work(this->_io);
				this->_io.run();
			}));
		spawn(_io, [this](boost::asio::yield_context yield) {
			this->LoopTimer(yield);
			}, boost::coroutines::attributes(1024 * 128));// 128 KB	
	}
	else
	{
		assert(false);
	}
}

void HttpClient::Get(const std::string& url, const std::string& uri, const HttpResponseCallBack& cb)
{
	this->_io.post([=]()
		{
			std::shared_ptr<HttpRequest> req = std::make_shared<HttpRequest>(this, this->_io, url, uri, cb);
			if (_processing_requestes.find(req.get()) == _processing_requestes.end())
			{
				req->Go();
				_processing_requestes.insert(make_pair(req.get(), req));
			}
			else
			{
				//memory error
				cb(0, "");
			}
		});
}


int main(int argc, char* argv[])
{
	int max_num = 0;
	HttpClient client;

	client.AsyncRun();

	/*client.Get("192.168.93.116", "/serverlist.txt", [&](int code, std::string response)
	{
	max_num++;
	cout << code << " " << response << ":" << response.size() << endl;
	});
	*/

	//http://ysdktest.qq.com/auth/wx_check_token

	client.Get("ysdktest.qq.com", "/auth/qq_check_token?timestamp=5665&appid=100703379&sig=565&openid=465465&openkey=45654656", [&](int code, std::string response)
		{
			max_num++;
			std::cout << code << " " << response << ":" << response.size() << std::endl;
		}
	);
	//	client._io.run();

	//	boost::asio::io_service io_service1;
	//HttpRequest c(io_service1, "", "/serverlist.txt");
	//	io_service1.run();
	system("pause");
	return 0;

	/*std::cout << "IP addresses: \n";
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::resolver resolver(io_service);
	boost::asio::ip::tcp::resolver::query query("jdhcr.com", "80");


	for (boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query);
	i != boost::asio::ip::tcp::resolver::iterator();
	++i)
	{
	boost::asio::ip::tcp::endpoint end = *i;
	std::cout << end.address() << ' ';

	boost::asio::io_service io_service1;
	client c(io_service1, end.address().to_v4().to_string(), "/serverlist.txt");
	io_service.run();

	}
	std::cout << '\n';
	*/

	/*try
	{

	boost::asio::io_service io_service;
	client c(io_service, "192.168.93.116","/index.html");
	io_service.run();
	}
	catch (std::exception& e)
	{
	std::cout << "Exception: " << e.what() << "\n";
	}
	*/
	system("pause");

	return 0;
}
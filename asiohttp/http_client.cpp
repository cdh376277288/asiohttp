/*
 * ��飺����Boost����http��Post/Get����
 * ���ߣ�cdh
 * Boost�汾��boost_1_73_0
 */
#include "http_client.h"

#include <boost/bind.hpp>

#include "http_mark.h"

cdh::HttpClient::HttpClient(boost::asio::io_service& io_service)
	: resolver_(io_service), socket_(io_service)
{
}

cdh::HttpClient::~HttpClient()
{
}

int cdh::HttpClient::post(const std::string& url)
{
	handle_request_resolve(url, HttpBase::buildPostRequest);
	return HTTP_SUCCESS;
}

int cdh::HttpClient::get(const std::string& url)
{
	handle_request_resolve(url, HttpBase::buildGetRequest);
	return HTTP_SUCCESS;
}

void cdh::HttpClient::handle_request_resolve(const std::string& url, pBuildRequest func)
{
	try
	{
		responseData_.clear();
		// ����URL
		std::string server, port, path;
		parseUrl(url, server, port, path);

		std::ostream request_stream(&request_);

		// �ϳ�����
		func(server, path, request_stream);

		// ���������ַ\�˿�
		boost::asio::ip::tcp::resolver::query query(server, port);
		resolver_.async_resolve(query, boost::bind(&cdh::HttpClient::handle_resolve, this, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
	}
	catch (std::exception& e)
	{
		socket_.close();
		std::cout << "Exception: " << e.what() << "\n";
	}
	return;
}

void cdh::HttpClient::handle_resolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	if (err)
	{
		std::cout << "Error: " << err << "\n";
		return;
	}

	// ��������
	boost::asio::async_connect(socket_, endpoint_iterator, boost::bind(&cdh::HttpClient::handle_connect, this, boost::asio::placeholders::error));
}

void cdh::HttpClient::handle_connect(const boost::system::error_code& err)
{
	if (err)
	{
		std::cout << "Error: " << err << "\n";
		return;
	}

	// ����request����
	boost::asio::async_write(socket_, request_, boost::bind(&cdh::HttpClient::handle_write_request, this, boost::asio::placeholders::error));
}

void cdh::HttpClient::handle_write_request(const boost::system::error_code& err)
{
	if (err)
	{
		std::cout << "Error: " << err << "\n";
		return;
	}

	// �첽���������ݵ�response_��ֱ������Э����� \r\n Ϊֹ
	boost::asio::async_read_until(socket_, response_, "\r\n", boost::bind(&cdh::HttpClient::handle_read_status_line, this, boost::asio::placeholders::error));
}

void cdh::HttpClient::handle_read_status_line(const boost::system::error_code& err)
{
	if (err)
	{
		std::cout << "Error: " << err << "\n";
		return;
	}

	// ����buff
	std::istream response_stream(&response_);
	unsigned int status_code;
	std::string http_version, status_message;
	response_stream >> http_version;
	response_stream >> status_code;
	std::getline(response_stream, status_message);

	// �˶��Ƿ�����ȷ����
	if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
		std::cout << "�������Ӧ����\n";
		return;
	}
	if (status_code != 200) {
		std::cout << "��������Ӧ��״̬��: " << status_code << "\n";
		return;
	}

	// ��ȡ��Ӧͷ,ֱ������Э����� \r\n\r\n Ϊֹ
	boost::asio::async_read_until(socket_, response_, "\r\n\r\n", boost::bind(&cdh::HttpClient::handle_read_headers, this, boost::asio::placeholders::error));
}

void cdh::HttpClient::handle_read_headers(const boost::system::error_code& err)
{
	if (err)
	{
		std::cout << "Error: " << err << "\n";
		return;
	}
	// �����Ӧͷ
	std::istream response_stream(&response_);
	std::string header;

	while (std::getline(response_stream, header) && header != "\r")
	{
#ifdef _DEBUG
		std::cout << header << "\n";
#endif
	}

#ifdef _DEBUG
	std::cout << "\n";
#endif

	// д������ʣ�������
	if (response_.size() > 0)
	{
		boost::asio::streambuf::const_buffers_type cbt = response_.data();
		responseData_ += std::string(boost::asio::buffers_begin(cbt), boost::asio::buffers_end(cbt));
#ifdef _DEBUG
		std::cout << &response_;
#endif
	}

	// ��ʼ��ȡʣ����������
	boost::asio::async_read(socket_, response_, boost::asio::transfer_at_least(1), boost::bind(&cdh::HttpClient::handle_read_content, this, boost::asio::placeholders::error));
}

void cdh::HttpClient::handle_read_content(const boost::system::error_code& err)
{
	if (!err)
	{
		// �������������
		boost::asio::streambuf::const_buffers_type cbt = response_.data();
		responseData_ += std::string(boost::asio::buffers_begin(cbt), boost::asio::buffers_end(cbt));
#ifdef _DEBUG
		std::cout << &response_;
#endif

		// ������ȡʣ�����ݣ�ֱ������EOF
		boost::asio::async_read(socket_, response_, boost::asio::transfer_at_least(1), boost::bind(&cdh::HttpClient::handle_read_content, this, boost::asio::placeholders::error));
	}
	else if (err != boost::asio::error::eof)
	{
		std::cout << "Error: " << err << "\n";
	}
	else
	{
		socket_.close();
		resolver_.cancel();
		std::cout << "��ȡ��Ӧ�������." << std::endl;
	}
}

std::string cdh::post(std::string url) {
	boost::asio::io_service io;
	cdh::HttpClient c(io);
	c.post(url);
	io.run();
	return c.getResponse();
}

std::string cdh::get(std::string url) {
	boost::asio::io_service io;
	cdh::HttpClient c(io);
	c.get(url);
	io.run();
	return c.getResponse();
}
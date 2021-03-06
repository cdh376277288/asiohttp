/*
 * 简介：利用Boost发送http的Post/Get请求
 * 作者：cdh
 * Boost版本：boost_1_73_0
 */

#ifndef __CDH_HTTP_CLIENT_H__
#define __CDH_HTTP_CLIENT_H__

#include "http_base.h"

#include <boost/asio.hpp>

namespace cdh {

	class HttpClient :
		public HttpBase {

	public:

		HttpClient(boost::asio::io_service& io_service);

		virtual ~HttpClient();

		/*
		* 发送Post请求
		*/
		virtual int post(const std::string& url);

		/*
		* 发送get请求
		*/
		virtual int get(const std::string& url);

		int test();

		virtual std::string getResponse(void) {
			return responseData_;
		}

	private:
		// 建立请求
		void handle_request_resolve(const std::string& url, pBuildRequest func);

		// 解析后
		void handle_resolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

		// 连接后
		void handle_connect(const boost::system::error_code& err);

		// 发送请求后
		void handle_write_request(const boost::system::error_code& err);

		// 读取响应后
		void handle_read_status_line(const boost::system::error_code& err);

		// 读取响应头后
		void handle_read_headers(const boost::system::error_code& err);

		// 读取正文数据后
		void handle_read_content(const boost::system::error_code& err);
	private:

		// 解析器
		boost::asio::ip::tcp::resolver resolver_;
		// 套接字
		boost::asio::ip::tcp::socket socket_;
		// 请求缓冲区
		boost::asio::streambuf request_;
		// 响应缓冲区
		boost::asio::streambuf response_;
		// 响应数据
		std::string responseData_;

	};
	// Post请求 消息范例 url:80/openapi/getversion[Post请求数据表格]
	std::string post(std::string url);



	std::string get(std::string url);


}
#endif // __CDH_HTTP_CLIENT_H__
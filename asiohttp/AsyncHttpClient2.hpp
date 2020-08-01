#pragma once

#include <map>
#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "HttpProtocol.hpp"

namespace Http
{
	///---------------------------------------------
	/// 基于asio的异步http client
	/// --不支持https
	/// ---cbh
	///---------------------------------------------
	class AsyncClient2 : public boost::enable_shared_from_this<AsyncClient2> {
	public:
		typedef boost::shared_ptr<AsyncClient2> Ptr;
		typedef boost::function<void(AsyncClient2::Ptr)> Callback;
	public:
		AsyncClient2(boost::asio::io_service& ioservice);
		~AsyncClient2();

		bool Parse(const std::string& url);

		void Get();
		void Get(Callback callback);
		void Post();
		void Post(Callback callback);
		void Post(const std::string& postData);
		void Post(const std::string& postData, Callback callback);

		bool IsSucceed();
		const std::string& GetResponse();
		const std::string& GetErrorMessage();
	protected:
		void Perform(const std::string& requestType, Callback callback, const std::string& requestData);
		void HandleResolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
		void HandleConnect(const boost::system::error_code& err);
		void HandleWriteFinish(const boost::system::error_code& err);
		void PostFinish();

		void StartRead();
		void HandleRead(size_t bytes_transferred, const boost::system::error_code& err);
	protected:
		Callback m_vCallback;
		boost::asio::io_service& m_rIoService;
	protected:
		int m_nPort;
		std::string m_sHost;
		std::string m_sQueryUrl;
		std::string m_sErrorInfo;
		std::string m_sRequestData;
		std::string m_sResponseData;
	protected:
		Protocol m_vResponse;
		boost::array<char, 4096> m_aryReceiveBuffer;
	protected:
		boost::asio::ip::tcp::resolver m_mResolver;
		boost::asio::ip::tcp::socket m_mSocket;
	};
}

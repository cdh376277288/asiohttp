#pragma once

#include <map>
#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>

namespace Http
{
	///---------------------------------------------
	/// 基于asio的异步http client
	/// --不支持https
	/// ---cbh
	///---------------------------------------------
	class AsyncClient : public boost::enable_shared_from_this<AsyncClient> {
	public:
		typedef boost::shared_ptr<AsyncClient> Ptr;
		typedef boost::function<void(AsyncClient::Ptr)> Callback;
	public:
		AsyncClient(boost::asio::io_service& ioservice);
		~AsyncClient();

		bool Parse(const std::string& url);
		void SetServer(const std::string& host, int port);
		void SetQueryUrl(const std::string& query);
		void AddQueryValue(const std::string& name, const std::string& value);
		void AddHeaderValue(const std::string& name, const std::string& value);

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
		void Perform(const std::string& requestType, Callback callback);
		void HandleResolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
		void HandleConnect(const boost::system::error_code& err);
		void HandleWriteFinish(const boost::system::error_code& err);
		void HandleReadStatusCodeFinish(const boost::system::error_code& err);
		void HandleReadHeadersFinish(const boost::system::error_code& err);
		void HandleReadTrunkedHeaderFinish(const boost::system::error_code& err);
		void HandleReadTrunkedContentFinish(const boost::system::error_code& err);
		void HandleReadContentFinish(const boost::system::error_code& err);
		void PostFinish();
	protected:
		Callback m_vCallback;
		boost::asio::io_service& m_rIoService;
	protected:
		int m_nPort;
		std::string m_sHost;
		std::string m_sPort;
		std::string m_sQueryUrl;
		std::string m_sRequestType;
		std::string m_sErrorInfo;
		std::string m_sRequestData;
		std::string m_sResponseData;
		std::map<std::string, std::string> m_mQueryValues;
		std::map<std::string, std::string> m_mQueryHeaders;
	protected:
		boost::asio::ip::tcp::resolver m_mResolver;
		boost::asio::ip::tcp::socket m_mSocket;
		boost::asio::streambuf m_mRequest;
		boost::asio::streambuf m_mResponse;
	protected:
		unsigned int m_nResponseLength;
		unsigned int m_nTrunkedLength;
	};
}
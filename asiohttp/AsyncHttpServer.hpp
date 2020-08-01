#pragma once
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/array.hpp>
#include "HttpProtocol.hpp"

namespace Http
{
	class AsyncServer;

	class AsyncServerConnection : public boost::enable_shared_from_this<AsyncServerConnection> {
	public:
		typedef boost::shared_ptr<AsyncServerConnection> Ptr;

		AsyncServerConnection(boost::asio::io_service& ioservice, boost::function<void(AsyncServerConnection::Ptr)> callback);
	public:
		boost::asio::ip::tcp::socket& socket() {
			return m_mSocket;
		}

		void Start();
		void WriteResponse(const std::string& response);
		void WriteResponse(Protocol& response);

		inline Protocol& GetRequest() {
			return m_vRequest;
		}
	private:
		void HandleWriteFinish(const boost::system::error_code& err);
		void HandleRead(size_t bytes_transferred, const boost::system::error_code& err);
	private:
		boost::asio::io_service& m_rIoService;
		boost::asio::ip::tcp::socket m_mSocket;
	private:
		Protocol m_vRequest;
		boost::function<void(AsyncServerConnection::Ptr)> m_vCallback;
		boost::array<char, 4096> m_aryReceiveBuffer;
		std::string m_sResponse;
	};

	class AsyncServer : public boost::enable_shared_from_this<AsyncServer> {
	public:
		typedef boost::shared_ptr<AsyncServer> Ptr;
	public:
		AsyncServer(boost::asio::io_service& ioservice, boost::function<void(AsyncServerConnection::Ptr)> callback);
	public:
		void Startup(const std::string& host, unsigned short port);
	private:
		void HandleResolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
		void StartAccept();
		void HandleAccept(const boost::system::error_code& err);
	private:
		std::string m_sErrorInfo;
	private:
		boost::asio::io_service& m_rIoService;
		boost::asio::ip::tcp::resolver m_mResolver;
		boost::asio::ip::tcp::acceptor m_mAcceptor;
	private:
		boost::shared_ptr<AsyncServerConnection> m_mNewConnection;
		boost::function<void(AsyncServerConnection::Ptr)> m_vCallback;
	};
}
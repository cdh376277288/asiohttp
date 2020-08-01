#include "AsyncHttpServer.hpp"
#include <boost/bind.hpp>

namespace Http
{
	using namespace boost::asio;
	using namespace boost::asio::ip;

	AsyncServerConnection::AsyncServerConnection(boost::asio::io_service& ioservice, boost::function<void(AsyncServerConnection::Ptr)> callback) : m_rIoService(ioservice)
		, m_mSocket(ioservice)
		, m_vCallback(callback)
	{
	}

	void AsyncServerConnection::Start() {
		m_mSocket.async_read_some(boost::asio::buffer(m_aryReceiveBuffer),
			boost::bind(&AsyncServerConnection::HandleRead, this->shared_from_this(), boost::asio::placeholders::bytes_transferred, boost::asio::placeholders::error));
	}

	void AsyncServerConnection::HandleRead(size_t bytes_transferred, const boost::system::error_code& err) {
		if (err) {
			return;
		}

		if (bytes_transferred > 0) {
			int remain = (int)bytes_transferred;
			while (remain > 0)
			{
				int pos = bytes_transferred - remain;
				if (m_vRequest.Parse(&m_aryReceiveBuffer[pos], bytes_transferred, &remain)) {
					this->m_vCallback(this->shared_from_this());
					m_vRequest.Clear();
				}
				else {
					break;
				}
			}
		}
		this->Start();
	}

	void AsyncServerConnection::WriteResponse(const std::string& response)
	{
		Protocol res;
		res.WriteResponseHeader(200, "OK");
		res.WriteHeader(Consts::HEADER_CONNECTION, "close");
		res.WriteHeader(Consts::HEADER_CONTENT_LENGTH, (int)response.length());
		res.WriteContent(response.c_str(), (int)response.length());
		m_sResponse.clear();
		m_sResponse.append(res.GetBuf(), res.GetLen());

		m_mSocket.async_send(boost::asio::buffer(m_sResponse),
			boost::bind(&AsyncServerConnection::HandleWriteFinish, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void AsyncServerConnection::WriteResponse(Protocol& response)
	{
		m_sResponse.clear();
		m_sResponse.append(response.GetBuf(), response.GetLen());

		m_mSocket.async_send(boost::asio::buffer(m_sResponse),
			boost::bind(&AsyncServerConnection::HandleWriteFinish, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void AsyncServerConnection::HandleWriteFinish(const boost::system::error_code& err) {
	}

	AsyncServer::AsyncServer(boost::asio::io_service& ioservice, boost::function<void(AsyncServerConnection::Ptr)> callback)
		: m_rIoService(ioservice)
		, m_mResolver(ioservice)
		, m_mAcceptor(ioservice)
	{
		m_vCallback = callback;
	}

	void AsyncServer::Startup(const std::string& host, unsigned short port) {
        tcp::endpoint epoint = tcp::endpoint(boost::asio::ip::address_v4::from_string(host.c_str()), port);
		m_mAcceptor.open(epoint.protocol());
		m_mAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		m_mAcceptor.bind(epoint);
		m_mAcceptor.listen();

		this->StartAccept();
	}

	void AsyncServer::HandleResolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
	{
		if (err) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			return;
		}

		m_mAcceptor.open(endpoint_iterator->endpoint().protocol());
		m_mAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		m_mAcceptor.bind(endpoint_iterator->endpoint());
		m_mAcceptor.listen();

		this->StartAccept();
	}

	void AsyncServer::StartAccept() {
		m_mNewConnection.reset(new AsyncServerConnection(m_rIoService, m_vCallback));
		m_mAcceptor.async_accept(m_mNewConnection->socket(),
			boost::bind(&AsyncServer::HandleAccept, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void AsyncServer::HandleAccept(const boost::system::error_code& err) {
		m_mNewConnection->Start();
		StartAccept();
	}
}
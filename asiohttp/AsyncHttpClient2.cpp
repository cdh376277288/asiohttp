#include <boost/bind.hpp>
#include "AsyncHttpClient2.hpp"
#include "HttpUtils.hpp"

namespace Http
{
	AsyncClient2::AsyncClient2(boost::asio::io_service& ioservice)
		: m_rIoService(ioservice)
		, m_mResolver(ioservice)
		, m_mSocket(ioservice) {

	}

	AsyncClient2::~AsyncClient2() {

	}

	bool AsyncClient2::IsSucceed() {
		return this->m_sErrorInfo.empty();
	}

	const std::string& AsyncClient2::GetResponse() {
		if (m_sResponseData.empty())
			m_sResponseData.append(m_vResponse.GetContent(), m_vResponse.GetContentLength());
		return m_sResponseData;
	}

	const std::string& AsyncClient2::GetErrorMessage() {
		return this->m_sErrorInfo;
	}

	bool AsyncClient2::Parse(const std::string& url_) {
		
		std::string path, query_params;
		if (!Http::Parse(url_, m_sHost, m_nPort, path, query_params))
			return false;
		m_sQueryUrl = path + "?" + query_params;
		return true;
	}

	void AsyncClient2::Get()
	{
		this->Perform("GET", Callback(),"");
	}

	void AsyncClient2::Get(AsyncClient2::Callback callback)
	{
		this->Perform("GET", callback, "");
	}

	void AsyncClient2::Post()
	{
		this->Perform("POST", Callback(), "");
	}

	void AsyncClient2::Post(AsyncClient2::Callback callback)
	{
		this->Perform("POST", callback, "");
	}

	void AsyncClient2::Post(const std::string& postData) {
		this->Perform("POST", AsyncClient2::Callback(), postData);
	}

	void AsyncClient2::Post(const std::string& postData, Callback callback) {
		this->Perform("POST", callback, postData);
	}

	void AsyncClient2::PostFinish() {
		if (this->m_vCallback)
			m_rIoService.post(boost::bind(this->m_vCallback, this->shared_from_this()));
	}

	void AsyncClient2::Perform(const std::string& requestType, Callback callback, const std::string& requestData)
	{
		using boost::asio::ip::tcp;

		if (m_sHost.empty())
			return;

		m_vCallback = callback;
		if (m_sQueryUrl.empty())
			m_sQueryUrl.append("/");

		m_vResponse.Clear();

		Protocol request;
		request.WriteRequestHeader(requestType.c_str(), m_sQueryUrl.c_str());
		request.WriteHeader(Consts::HEADER_ACCEPT, "*/*");
		request.WriteHeader(Consts::HEADER_HOST, m_sHost.c_str());
		request.WriteHeader(Consts::HEADER_CONTENT_LENGTH, (int)requestData.length());
		request.WriteContent(requestData.c_str(), (int)requestData.length());
		m_sRequestData.clear();
		m_sRequestData.append(request.GetBuf(), request.GetLen());

		if (this->m_mSocket.is_open()) {
			this->HandleConnect(boost::system::error_code());
		}
		else {
			std::string sPort = IntegerToString(m_nPort);
			tcp::resolver::query query(m_sHost, sPort);
			m_mResolver.async_resolve(query,
				boost::bind(&AsyncClient2::HandleResolve, this->shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::iterator));
		}
	}

	void AsyncClient2::HandleResolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
	{
		if (err) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			this->PostFinish();
			return;
		}

		boost::asio::async_connect(this->m_mSocket, endpoint_iterator,
			boost::bind(&AsyncClient2::HandleConnect, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void AsyncClient2::HandleConnect(const boost::system::error_code& err) {
		if (err) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			this->PostFinish();
			return;
		}

		m_mSocket.async_send(boost::asio::buffer(m_sRequestData),
			boost::bind(&AsyncClient2::HandleWriteFinish, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void AsyncClient2::HandleWriteFinish(const boost::system::error_code& err) {
		if (err) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			this->PostFinish();
			return;
		}
		this->StartRead();
	}

	void AsyncClient2::StartRead() {
		m_mSocket.async_read_some(boost::asio::buffer(m_aryReceiveBuffer),
			boost::bind(&AsyncClient2::HandleRead, this->shared_from_this(), boost::asio::placeholders::bytes_transferred,boost::asio::placeholders::error));
	}

	void AsyncClient2::HandleRead(size_t bytes_transferred, const boost::system::error_code& err) {
		if (err) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			this->PostFinish();
			return;
		}

		if (bytes_transferred > 0) {
			if (m_vResponse.Parse(&m_aryReceiveBuffer[0], bytes_transferred)) {
				this->PostFinish();
				return;
			}
		}
		this->StartRead();
	}
}
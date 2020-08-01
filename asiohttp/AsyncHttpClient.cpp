#include <boost/bind.hpp>
#include "AsyncHttpClient.hpp"
#include "HttpUtils.hpp"

namespace Http
{
	static std::string s_HttpHeaderContentLength("Content-Length");
	static std::string s_HttpHeaderTransferEncoding("Transfer-Encoding");
	static std::string s_HttpUrlStart("http://");

	static std::string ExtractHeaderValue(const std::string& header, const std::string& headerName) {
		// ����: "Content-Length: 45356\r"
		// substr  +2 Ϊ���� ð��+�ո�,-3Ϊȥ�� ð��+�ո�+\r�ĳ���
		return header.substr(headerName.length() + 2, header.length() - headerName.length() - 3);
	}

	AsyncClient::AsyncClient(boost::asio::io_service& ioservice)
		: m_rIoService(ioservice)
		, m_mResolver(ioservice)
		, m_mSocket(ioservice)
		, m_nResponseLength(0)
		, m_nTrunkedLength(0) {

	}

	AsyncClient::~AsyncClient() {

	}

	bool AsyncClient::IsSucceed() {
		return this->m_sErrorInfo.empty();
	}

	const std::string& AsyncClient::GetResponse() {
		return this->m_sResponseData;
	}

	const std::string& AsyncClient::GetErrorMessage() {
		return this->m_sErrorInfo;
	}

	bool AsyncClient::Parse(const std::string& url_) {
		std::string url = url_;
		if (url.substr(0, s_HttpUrlStart.length()) != s_HttpUrlStart)
			return false;
		url.erase(0, s_HttpUrlStart.length());

		bool ignorePort = false;
		// parse host
		size_t pos = url.find(":");
		if (pos == std::string::npos) {
			pos = url.find('/');
			ignorePort = true;
		}
		m_sHost = url.substr(0, pos);
		url = url.erase(0, pos);
		if (!ignorePort) {
			url.erase(0, 1);
		}

		// parse port
		m_nPort = 80;
		if (!ignorePort) {
			pos = url.find('/');
			if (pos == std::string::npos) {
				pos = url.length();
			}
			std::string portStr = url.substr(0, pos);
			url = url.erase(0, pos);
			m_nPort = StringToInteger(portStr);
		}

		// query string
		pos = url.find('?');
		if (pos != std::string::npos) {
			m_sQueryUrl = url.substr(0, pos);
			url.erase(0, pos + 1);

			ParseQueryValues(url, m_mQueryValues);
		}
		else {
			m_sQueryUrl = url;
		}
		return true;
	}

	void AsyncClient::SetServer(const std::string& host, int port)
	{
		m_sHost = host;
		m_nPort = port;
	}

	void AsyncClient::SetQueryUrl(const std::string& query)
	{
		m_sQueryUrl = query;
	}

	void AsyncClient::AddQueryValue(const std::string& name, const std::string& value)
	{
		m_mQueryValues[name] = value;
	}

	void AsyncClient::AddHeaderValue(const std::string& name, const std::string& value) {
		m_mQueryHeaders[name] = value;
	}

	void AsyncClient::Get()
	{
		this->Perform("GET", Callback());
	}

	void AsyncClient::Get(AsyncClient::Callback callback)
	{
		this->Perform("GET", callback);
	}

	void AsyncClient::Post()
	{
		this->Perform("POST", Callback());
	}

	void AsyncClient::Post(AsyncClient::Callback callback)
	{
		this->Perform("POST", callback);
	}

	void AsyncClient::Post(const std::string& postData) {
		m_sRequestData = postData;
		this->Post();
	}

	void AsyncClient::Post(const std::string& postData, Callback callback) {
		m_sRequestData = postData;
		this->Perform("POST", callback);
	}

	void AsyncClient::PostFinish() {
		if (this->m_vCallback)
			m_rIoService.post(boost::bind(this->m_vCallback, this->shared_from_this()));
	}

	void AsyncClient::Perform(const std::string& requestType, Callback callback)
	{
		using boost::asio::ip::tcp;

		if (m_sHost.empty())
			return;

		m_vCallback = callback;
		m_sRequestType = requestType;
		m_nResponseLength = 0;

		if (m_sQueryUrl.empty())
			m_sQueryUrl = "/";

		if (m_sQueryUrl[0] != '/') {
			m_sQueryUrl.insert(0, "/");
		}

		m_sPort = IntegerToString(m_nPort);

		if (this->m_mSocket.is_open()) {
			this->HandleConnect(boost::system::error_code());
		}
		else {
			tcp::resolver::query query(m_sHost, m_sPort);
			m_mResolver.async_resolve(query,
				boost::bind(&AsyncClient::HandleResolve, this->shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::iterator));
		}
	}

	void AsyncClient::HandleResolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
	{
		if (err) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			this->PostFinish();
			return;
		}

		boost::asio::async_connect(this->m_mSocket, endpoint_iterator,
			boost::bind(&AsyncClient::HandleConnect, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void AsyncClient::HandleConnect(const boost::system::error_code& err) {
		if (err) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			this->PostFinish();
			return;
		}

		std::string querystr(m_sQueryUrl);
		if (m_mQueryValues.size() > 0) {
			querystr.append("?");
			for (std::map<std::string, std::string>::iterator iter = m_mQueryValues.begin(); iter != m_mQueryValues.end(); ++iter) {
				querystr.append(UrlEncode(iter->first)).append("=").append(UrlEncode(iter->second)).append("&");
			}
			querystr.resize(querystr.length() - 1);
		}

		m_mSocket.set_option(boost::asio::ip::tcp::no_delay(true));

		std::ostream request_stream(&m_mRequest);
		request_stream << m_sRequestType << " " << querystr << " HTTP/1.1\r\n";
		request_stream << "Host: " << m_sHost << "\r\n";
		request_stream << "Accept: */*\r\n";
		request_stream << "Content-Length: " << m_sRequestData.length() << "\r\n";

		for (std::map<std::string, std::string>::iterator iter = m_mQueryHeaders.begin(); iter != m_mQueryHeaders.end(); ++iter) {
			request_stream << iter->first << ": " << iter->second << "\r\n";
		}

		//	request_stream << "Content-Type: application/json\r\n";
		request_stream << "Connection: close\r\n";
		// 	request_stream << "Connection: keep-alive\r\n";
		request_stream << "\r\n";
		request_stream << m_sRequestData;

		boost::asio::async_write(m_mSocket, m_mRequest,
			boost::bind(&AsyncClient::HandleWriteFinish, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void AsyncClient::HandleWriteFinish(const boost::system::error_code& err) {
		if (err) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			this->PostFinish();
			return;
		}
		boost::asio::async_read_until(m_mSocket, m_mResponse, "\r\n",
			boost::bind(&AsyncClient::HandleReadStatusCodeFinish, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void AsyncClient::HandleReadStatusCodeFinish(const boost::system::error_code& err) {
		if (err) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			this->PostFinish();
			return;
		}

		std::string httpVersion;
		std::string statusMessage;
		unsigned int statusCode;

		std::istream response_stream(&m_mResponse);
		response_stream >> httpVersion;
		response_stream >> statusCode;
		std::getline(response_stream, statusMessage);
		if (!response_stream || httpVersion.substr(0, 5) != "HTTP/")
		{
			m_sErrorInfo.append(__FUNCTION__).append(":").append("invalid response ").append(httpVersion);
			this->PostFinish();
			return;
		}
		if (statusCode != 200)
		{
			m_sErrorInfo.append(__FUNCTION__).append(":").append("response returned with status code ").append(IntegerToString(statusCode));
			this->PostFinish();
			return;
		}
		boost::asio::async_read_until(m_mSocket, m_mResponse, "\r\n\r\n",
			boost::bind(&AsyncClient::HandleReadHeadersFinish, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void AsyncClient::HandleReadHeadersFinish(const boost::system::error_code& err) {
		if (err) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			this->PostFinish();
			return;
		}

		bool trunkedEncoding = false;
		std::string header;
		std::istream response_stream(&m_mResponse);
		while (std::getline(response_stream, header) && header != "\r") {
			if (header.substr(0, s_HttpHeaderContentLength.length()) == s_HttpHeaderContentLength) {
				//"Content-Length: 45356\r"
				m_nResponseLength = StringToInteger(ExtractHeaderValue(header, s_HttpHeaderContentLength));
			}
			else if (header.substr(0, s_HttpHeaderTransferEncoding.length()) == s_HttpHeaderTransferEncoding) {
				std::string headerValue = ExtractHeaderValue(header, s_HttpHeaderTransferEncoding);
				if (headerValue == "chunked") {
					trunkedEncoding = true;
				}
			}
		}

		if (m_nResponseLength <= 0 && !trunkedEncoding) {
			this->PostFinish();
			return;
		}

		if (trunkedEncoding) {
			boost::asio::async_read_until(m_mSocket, m_mResponse, "\r\n",
				boost::bind(&AsyncClient::HandleReadTrunkedHeaderFinish, this->shared_from_this(), boost::asio::placeholders::error));
			return;
		}
		boost::asio::async_read(m_mSocket, m_mResponse,
			boost::asio::transfer_at_least(m_nResponseLength), boost::bind(&AsyncClient::HandleReadContentFinish, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void AsyncClient::HandleReadTrunkedHeaderFinish(const boost::system::error_code& err) {
		if (err) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			this->PostFinish();
			return;
		}

		std::string trunkedSize;
		std::istream response_stream(&m_mResponse);
		if (!std::getline(response_stream, trunkedSize)) {
			m_sErrorInfo.append(__FUNCTION__).append(":read trunked size failed");
			this->PostFinish();
			return;
		}

		// remove \r, by cbh
		if (!trunkedSize.empty() && trunkedSize[trunkedSize.length()-1] == '\r') {
			trunkedSize.resize(trunkedSize.length() - 1);
		}

		// last trunk?, by cbh
		m_nTrunkedLength = ::strtol(trunkedSize.c_str(), NULL, 16);
		if (m_nTrunkedLength == 0) {
			this->PostFinish();
			return;
		}

		boost::asio::async_read(m_mSocket, m_mResponse,
			boost::asio::transfer_exactly(m_nTrunkedLength), boost::bind(&AsyncClient::HandleReadTrunkedContentFinish, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void AsyncClient::HandleReadTrunkedContentFinish(const boost::system::error_code& err) {
		if (err) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			this->PostFinish();
			return;
		}

		std::string trunkedData;
		std::istream response_stream(&m_mResponse);
		trunkedData.resize(m_nTrunkedLength);
		response_stream.read((char*)&trunkedData[0], m_nTrunkedLength);
		m_sResponseData.append(trunkedData);

		// read next chunked block , by cbh
		boost::asio::async_read_until(m_mSocket, m_mResponse, "\r\n",
			boost::bind(&AsyncClient::HandleReadTrunkedHeaderFinish, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void AsyncClient::HandleReadContentFinish(const boost::system::error_code& err) {
		if (err && err != boost::asio::error::eof) {
			m_sErrorInfo.append(__FUNCTION__).append(":").append(err.message());
			this->PostFinish();
			return;
		}

		std::istream response_stream(&m_mResponse);
		m_sResponseData.resize(m_nResponseLength);
		response_stream.read((char*)&m_sResponseData[0], m_nResponseLength);
		this->PostFinish();
	}
}
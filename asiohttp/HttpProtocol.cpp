#include "HttpProtocol.hpp"
#include "HttpUtils.hpp"
#include <string.h>

namespace Http
{
	namespace Consts
	{
		const char* LF = "\r\n";
		const char* LF2X = "\r\n\r\n";

		const char* METHOD_GET = "GET";
		const char* METHOD_PUT = "PUT";
		const char* METHOD_POST = "POST";

		const char* VERSION_10 = "HTTP/1.0";
		const char* VERSION_11 = "HTTP/1.1";

		const char* HEADER_HOST = "Host";
		const char* HEADER_USER_AGENT = "User-Agent";
		const char* HEADER_CONTENT_TYPE = "Content-Type";
		const char* HEADER_CONTENT_LENGTH = "Content-Length";
		const char* HEADER_CONTENT_ENCODING = "Content-Endcoding";
		const char* HEADER_TRANSFER_ENCODING = "Transfer-Encoding";

		const char* HEADER_ACCEPT = "Accept";
		const char* HEADER_ACCEPT_CHARSET = "Accept-Charset";
		const char* HEADER_ACCEPT_ENCODING = "Accept-Encoding";
		const char* HEADER_ACCEPT_LANGUAGE = "Accept-Language";

		const char* HEADER_COOKIE = "Cookie";
		const char* HEADER_CONNECTION = "Connection";
		const char* HEADER_AUTHORIZATION = "Authorization";

		const char* HEADER_SET_COOKIE = "Set-Cookie";
		const char* HEADER_REFERER = "Referer";

		const char* VALUE_CONNECTION_KEEP_ALIVE = "Keep-Alive";

		const char* HEADER_LOCATION = "Location";
	}

	Protocol::Protocol()
	{
		m_pBuf = 0;
		m_nLen = 0;
		m_bNeedReleaseBuf = true;
		m_nLastParsePosition = 0;
		m_nMode = PARSE_MODE_HEADER;
		Clear();
	}

	Protocol::~Protocol()
	{
		if (m_bNeedReleaseBuf)
		{
			free(m_pBuf);
		}
	}

	void Protocol::Reserve(int32 len)
	{
		if (m_nLen >= len)
			return;

		if (len < 1024)
			len = 1024;

		int newlen = 1024;
		while (newlen < len)
			newlen *= 2;

		char* oldBuf = m_pBuf;
		m_pBuf = (char*)realloc(m_pBuf, newlen);
		m_pBuf[m_nLen] = '\0';
		m_nLen = newlen;
		OnBufferRealloced(oldBuf, m_pBuf);
	}

	void Protocol::WriteBuffer(const char* buf)
	{
		WriteBuffer(buf, strlen(buf));
	}

	void Protocol::WriteBuffer(const char* buf, int32 len)
	{
		Reserve(m_nLenUsed + len + 1);
		memcpy(&m_pBuf[m_nLenUsed], buf, len);
		m_nLenUsed += len;
		m_pBuf[m_nLenUsed] = 0;
	}

	void Protocol::WriteHeader(const char* name, const char* value)
	{
		char buf[4096] = "";
		char* targetValue = NULL;
		WriteBuffer(name);
		WriteBuffer(": ");
		targetValue = &m_pBuf[m_nLenUsed];
		WriteBuffer(value);
		WriteBuffer(Consts::LF);

		OnHeaderSet(name, targetValue);
	}

	void Protocol::WriteHeader(const char* name, int32 value)
	{
		std::string str = Http::IntegerToString(value);
		WriteHeader(name, str.c_str());
	}

	bool Protocol::OnHeaderSet(const char* name, char* value)
	{
		if (strcmp(name, Consts::HEADER_CONTENT_LENGTH) == 0)
		{
			
			m_nContentLength = Http::StringToInteger(value);
			m_phContentLength = value;
		}
		else if (strcmp(name, Consts::HEADER_TRANSFER_ENCODING) == 0)
		{
			m_phTransferEncoding = value;
		}
		else if (strcmp(name, Consts::HEADER_CONTENT_TYPE) == 0)
		{
			m_phContentType = value;
		}
		else if (strcmp(name, Consts::HEADER_CONTENT_ENCODING) == 0)
		{
			m_phContentEndcoding = value;
		}
		else if (strcmp(name, Consts::HEADER_ACCEPT) == 0)
		{
			m_phAccept = value;
		}
		else if (strcmp(name, Consts::HEADER_ACCEPT_CHARSET) == 0)
		{
			m_phAcceptCharset = value;
		}
		else if (strcmp(name, Consts::HEADER_ACCEPT_ENCODING) == 0)
		{
			m_phAcceptEncoding = value;
		}
		else if (strcmp(name, Consts::HEADER_ACCEPT_LANGUAGE) == 0)
		{
			m_phAcceptLanguage = value;
		}
		else if (strcmp(name, Consts::HEADER_COOKIE) == 0)
		{
			m_phCookie = value;
		}
		else if (strcmp(name, Consts::HEADER_CONNECTION) == 0)
		{
			m_phConnection = value;
		}
		else if (strcmp(name, Consts::HEADER_HOST) == 0)
		{
			m_phHost = value;
		}
		else if (strcmp(name, Consts::HEADER_AUTHORIZATION) == 0)
		{
			m_phAuthorization = value;
		}
		else if (strcmp(name, Consts::HEADER_SET_COOKIE) == 0)
		{
			m_phSetCookie = value;
		}
		else if (strcmp(name, Consts::HEADER_REFERER) == 0)
		{
			m_phReferer = value;
		}
		else if (strcmp(name, Consts::HEADER_LOCATION) == 0)
		{
			m_phLocation = value;
		}
		else
		{
			return false;
		}
		return true;
	}

	void Protocol::OnBufferRealloced(char* oldbuf, char* newbuf)
	{
#define adjust_buffer(x) if(oldbuf && x != NULL) { x = &newbuf[x - oldbuf]; }
		adjust_buffer(m_phStatusCode);
		adjust_buffer(m_phStatusText);

		adjust_buffer(m_phMethod);
		adjust_buffer(m_phUrl);
		adjust_buffer(m_phVersion);
		adjust_buffer(m_phHost);
		adjust_buffer(m_phCookie);
		adjust_buffer(m_phConnection);
		adjust_buffer(m_phAccept);
		adjust_buffer(m_phAcceptEncoding);
		adjust_buffer(m_phAcceptLanguage);
		adjust_buffer(m_phAcceptCharset);
		adjust_buffer(m_phContentType);
		adjust_buffer(m_phContentLength);
		adjust_buffer(m_phTransferEncoding);
		adjust_buffer(m_phContentEndcoding);
		adjust_buffer(m_phAuthorization);

		adjust_buffer(m_phSetCookie);
		adjust_buffer(m_phCookie);
		adjust_buffer(m_phReferer);
		adjust_buffer(m_phLocation);
#undef adjust_buffer
	}

	void Protocol::Clear()
	{
		m_nLastParsePosition = 0;
		m_nMode = PARSE_MODE_HEADER;
		m_nLenUsed = 0;
		m_nContentLength = 0;
		m_phStatusCode = 0;
		m_phStatusText = 0;

		m_phMethod = 0;
		m_phUrl = 0;
		m_phVersion = 0;
		m_phHost = 0;
		m_phCookie = 0;
		m_phConnection = 0;
		m_phAccept = 0;
		m_phAcceptEncoding = 0;
		m_phAcceptLanguage = 0;
		m_phAcceptCharset = 0;
		m_phContentType = 0;
		m_phContentLength = 0;
		m_phTransferEncoding = 0;
		m_phContentEndcoding = 0;
		m_phAuthorization = 0;

		m_phSetCookie = 0;
		m_phCookie = 0;
		m_phReferer = 0;
		m_phLocation = 0;
	}

	bool Protocol::Parse(const char* buf, int32 len, int32* remainLen)
	{
		if (m_nMode == PARSE_MODE_OK)
		{
			if (remainLen != NULL)
				*remainLen = len;
			return true;
		}

		if (buf != NULL && len > 0)
		{
			WriteBuffer(buf, len);
		}

		if (m_pBuf == NULL)
			return false;

		int32 oldmode = PARSE_MODE_NONE;
		while (oldmode != m_nMode)
		{
			oldmode = m_nMode;

			switch (m_nMode)
			{
			case PARSE_MODE_HEADER:
				{
					char* content = strstr(&m_pBuf[m_nLastParsePosition], Consts::LF2X);
					if (content == NULL)
					{
						m_nLastParsePosition = m_nLenUsed;
						break;
					}

					char* line = m_pBuf;
					while (line < content)
					{
						char* nextline = strstr(line, Consts::LF);
						if (nextline == NULL)
							return false;

						bool result = true;
						if (line == m_pBuf)
							result = ParseProtocolHeader(line, nextline - line);
						else
							result = ParseHeader(line, nextline - line);

						if (!result)
							return false;

						line = nextline + 2;
					}

					line += 2; // last \r\n\

					m_nLastParsePosition = (int32)(line - m_pBuf);

					m_nMode = PARSE_MODE_CONTENT;

					const char* cst_chunked_str = "chunked";
					if (m_phTransferEncoding != 0 && memcmp(m_phTransferEncoding, cst_chunked_str, strlen(cst_chunked_str)) == 0)
					{
						m_nMode = PARSE_MODE_CHUNKED_LENGTH;
					}
				}
				break;
			case PARSE_MODE_CHUNKED_LENGTH:
				{
					char* nextline = strstr(&m_pBuf[m_nLastParsePosition], Consts::LF);
					if (nextline != 0)
					{
						m_nContentLength = HexStringToInteger(&m_pBuf[m_nLastParsePosition]);
						char* line = nextline + 2;

						m_nLastParsePosition = (int32)(line - m_pBuf);
						m_nMode = PARSE_MODE_CONTENT;
					}
				}
				break;
			case PARSE_MODE_CONTENT:
				{
					if (m_nLenUsed >= (m_nLastParsePosition + m_nContentLength))
					{
						m_nMode = PARSE_MODE_OK;

						if (remainLen)
							*remainLen = m_nLenUsed - (m_nLastParsePosition + m_nContentLength);
					}
				}
				break;
			}
		}

		return m_nMode == PARSE_MODE_OK;
	}

	bool Protocol::ParseHeader(char* header, int32 len)
	{
		char tmp = 0;
		std::swap(header[len], tmp);
		bool result = ParseHeader(header);
		std::swap(header[len], tmp);
		return result;
	}

	bool Protocol::ParseHeader(char* header)
	{
		char* value = strstr(header, ": ");
		if (value == NULL)
			return false;

		char tmp = 0;
		std::swap(header[value - header], tmp);
		OnHeaderSet(header, value + 2);
		std::swap(header[value - header], tmp);
		return true;
	}

	bool Protocol::ParseProtocolHeader(char* header, int32 len)
	{
		if (memcmp(header, "HTTP", 4) == 0)
		{
			m_phVersion = header;

			m_phStatusCode = strstr(m_phVersion, " ");
			if (m_phStatusCode == NULL)
				return false;
			m_phStatusCode += 1;

			m_phStatusText = strstr(m_phStatusCode, " ");
			if (m_phStatusText == NULL)
				return false;
			m_phStatusText += 1;
		}
		else
		{
			m_phMethod = header;

			m_phUrl = strstr(m_phMethod, " ");
			if (m_phUrl == NULL)
				return false;
			m_phUrl += 1;

			m_phVersion = strstr(m_phUrl, " ");
			if (m_phVersion == NULL)
				return false;
			m_phVersion += 1;
		}
		return true;
	}

	void Protocol::WriteRequestHeader(const char* method, const char* url /*= "/"*/, const char* version /*= VERSION_11*/)
	{
		m_phMethod = &m_pBuf[m_nLenUsed];
		WriteBuffer(method);
		WriteBuffer(" ");

		m_phUrl = &m_pBuf[m_nLenUsed];
		WriteBuffer(url);
		WriteBuffer(" ");

		m_phVersion = &m_pBuf[m_nLenUsed];
		WriteBuffer(version);
		WriteBuffer(Consts::LF);
	}

	void Protocol::WriteResponseHeader(int32 statusCode, const char* statusText, const char* version /*= HttpConsts::VERSION_11*/)
	{
		m_phVersion = &m_pBuf[m_nLenUsed];
		WriteBuffer(version);
		WriteBuffer(" ");

		m_phStatusCode = &m_pBuf[m_nLenUsed];
		
		std::string codebuf = Http::IntegerToString(statusCode);
		WriteBuffer(codebuf.c_str());
		WriteBuffer(" ");

		m_phStatusText = &m_pBuf[m_nLenUsed];
		WriteBuffer(statusText);
		WriteBuffer(Consts::LF);
	}

	void Protocol::WriteHeaderTerminator()
	{
		WriteBuffer(Consts::LF);
	}

	void Protocol::WriteContent(const void* buf, int32 len)
	{
		m_nContentLength = len;
		WriteHeaderTerminator();
		WriteBuffer((const char*)buf, len);
	}

	int Protocol::GetHeaderLength(char* header)
	{
		int count = 0;
		while (*header != ':' && *header != ' ' && *header != '\r' && *header != '\n')
		{
			++header;
			++count;
		}
		return count;
	}

	int Protocol::GetHeaderValueLength(char* header)
	{
		int count = 0;
		while (*header != ' ' && *header != '\r' && *header != '\n')
		{
			++header;
			++count;
		}
		return count;
	}

	std::string Protocol::GetInputUrl()
	{
		if (m_pBuf == NULL || m_nLen == 0)
			return "";

		std::string sInput = m_pBuf;
		std::string::size_type start = sInput.find("/");
		if (start != std::string::npos)
		{
			std::string::size_type end = sInput.find("?", start + 1);
			if (end == std::string::npos)
			{
				end = sInput.find(" ", start + 1);
			}
			return sInput.substr(start, end - start);
		}
		return "";
	}

	std::string Protocol::GetInputParam()
	{
		if (m_pBuf == NULL || m_nLen == 0)
			return "";

		std::string sInput = m_pBuf;
		if (sInput.find("GET /") != std::string::npos)
		{
			std::string::size_type start = sInput.find("?");
			std::string::size_type end = sInput.find(" HTTP", start + 1);
			if (start == std::string::npos || end == std::string::npos)
			{
				return "";
			}

			return sInput.substr(start + 1, end - start - 1);
		}
		else if (sInput.find("POST /") != std::string::npos)
		{
			std::string::size_type start = sInput.find("\r\n\r\n");
			if (start == std::string::npos)
			{
				return "";
			}

			return sInput.substr(start + 4);
		}
		return "";
	}
}
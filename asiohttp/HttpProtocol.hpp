#pragma once

#include <map>
#include <string>
#include <boost/noncopyable.hpp>

namespace Http
{
	typedef int int32;

	namespace Consts
	{
		extern const char* LF;
		extern const char* LF2X;

		extern const char* METHOD_GET;
		extern const char* METHOD_PUT;
		extern const char* METHOD_POST;

		extern const char* VERSION_10;
		extern const char* VERSION_11;

		extern const char* HEADER_HOST;
		extern const char* HEADER_USER_AGENT;
		extern const char* HEADER_CONTENT_TYPE;
		extern const char* HEADER_CONTENT_LENGTH;
		extern const char* HEADER_CONTENT_ENCODING;
		extern const char* HEADER_TRANSFER_ENCODING;

		extern const char* HEADER_ACCEPT;
		extern const char* HEADER_ACCEPT_CHARSET;
		extern const char* HEADER_ACCEPT_ENCODING;
		extern const char* HEADER_ACCEPT_LANGUAGE;

		extern const char* HEADER_COOKIE;
		extern const char* HEADER_CONNECTION;
		extern const char* HEADER_AUTHORIZATION;

		extern const char* VALUE_CONNECTION_KEEP_ALIVE;

		extern const char* HEADER_LOCATION;
	}

	class Protocol : public boost::noncopyable
	{
	public:
		Protocol();
		virtual ~Protocol();
	public:
		char* GetBuf() const { return m_pBuf; }
		int32 GetLen() const { return m_nLenUsed; }
		char* GetContent() const { return &m_pBuf[m_nLastParsePosition]; }
		int32 GetContentLength() const { return m_nContentLength; } 
	public:
		virtual void Clear();
		virtual bool Parse(const char* buf = NULL,int32 len = 0, int32* remainLen = NULL);
	public:
		virtual void Reserve(int32 len);
	public:
		static int GetHeaderLength(char* header);
		static int GetHeaderValueLength(char* header);
	public:
		std::string GetInputUrl();
		std::string GetInputParam();
	public:
		void WriteRequestHeader(const char* method, const char* url = "/", const char* version = Consts::VERSION_11);
		void WriteResponseHeader(int32 statusCode, const char* statusText, const char* version = Consts::VERSION_11);
		void WriteContent(const void* buf, int32 len);

		void WriteHeader(const char* name,const char* value);
		void WriteHeader(const char* name,int32 value);
		void WriteHeaderTerminator();
	protected:
		void WriteBuffer(const char* buf);
		void WriteBuffer(const char* buf, int32 len);
	protected:
		bool ParseHeader(char* header,int32 len);
		bool ParseHeader(char* header);
		bool ParseProtocolHeader(char* header,int32 len);
	protected:
		virtual bool OnHeaderSet(const char* name,char* value);
		virtual void OnBufferRealloced(char* oldbuf,char* newbuf);
	protected:
		char* m_pBuf;
		int32 m_nLen;
		int32 m_nLenUsed;
		int32 m_nContentLength;
		bool  m_bNeedReleaseBuf;
	public:
		enum 
		{
			PARSE_MODE_NONE,
			PARSE_MODE_HEADER,
			PARSE_MODE_CHUNKED_LENGTH,
			PARSE_MODE_CONTENT,
			PARSE_MODE_OK,
		};
		int32 m_nMode;
		int32 m_nLastParsePosition;
	public:
		char* m_phStatusCode;
		char* m_phStatusText;

		char* m_phMethod;
		char* m_phUrl;
		char* m_phVersion;
		char* m_phHost;
		char* m_phCookie;
		char* m_phConnection;
		char* m_phAccept;
		char* m_phAcceptEncoding;
		char* m_phAcceptLanguage;
		char* m_phAcceptCharset;
		char* m_phContentType;
		char* m_phContentLength;
		char* m_phTransferEncoding;
		char* m_phContentEndcoding;
		char* m_phAuthorization;

		char* m_phSetCookie;
		char* m_phReferer;
		char* m_phLocation;
	};
}

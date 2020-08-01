#include "HttpUtils.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace Http
{
	std::string IntegerToString(int s) {
		char buf[100] = "";
		snprintf(buf,sizeof(buf), "%d", s);
		return buf;
	}

	int StringToInteger(const std::string& s) {
		return ::atoi(s.c_str());
	}

	int HexStringToInteger(const std::string& s) {
		return ::strtol(s.c_str(), NULL, 16);
	}

	char Dec2HexChar(short int n) {
		if (0 <= n && n <= 9) {
			return char(short('0') + n);
		}
		else if (10 <= n && n <= 15) {
			return char(short('A') + n - 10);
		}
		else {
			return char(0);
		}
	}

	short int HexChar2Dec(char c) {
		if ('0' <= c && c <= '9') {
			return short(c - '0');
		}
		else if ('a' <= c && c <= 'f') {
			return (short(c - 'a') + 10);
		}
		else if ('A' <= c && c <= 'F') {
			return (short(c - 'A') + 10);
		}
		else {
			return -1;
		}
	}

	std::string UrlEncode(const std::string& URL) {
		std::string strResult = "";
		for (unsigned int i = 0; i < URL.size(); i++)
		{
			char c = URL[i];
			if (
				('0' <= c && c <= '9') ||
				('a' <= c && c <= 'z') ||
				('A' <= c && c <= 'Z') ||
				// 				c=='/' || c=='.'
				c == '_' || c == '.'
				) {
				strResult += c;
			}
			else {
				int j = (short int)c;
				if (j < 0)
				{
					j += 256;
				}
				int i1, i0;
				i1 = j / 16;
				i0 = j - i1 * 16;
				strResult += '%';
				strResult += Dec2HexChar(i1);
				strResult += Dec2HexChar(i0);
			}
		}

		return strResult;
	}

	std::string UrlDecode(const std::string& URL) {
		std::string result = "";
		for (unsigned int i = 0; i < URL.size(); i++) {
			char c = URL[i];
			if (c != '%') {
				result += c;
			}
			else {
				if (i + 2 < URL.size())
				{
					char c1 = URL[++i];
					char c0 = URL[++i];
					int num = 0;
					num += HexChar2Dec(c1) * 16 + HexChar2Dec(c0);
					result += char(num);
				}
			}
		}
		return result;
	}

	void ParseQueryValues(const std::string& str, std::map<std::string, std::string>& values) {
		size_t offset = 0;
		while (offset < str.length()) {
			size_t pos = str.find('=', offset);
			if (pos == std::string::npos)
				break;
			size_t pos2 = str.find_first_of('&', offset);
			if (pos2 == std::string::npos)
				pos2 = str.length();

			std::string key = str.substr(offset, pos - offset);
			std::string val = str.substr(pos + 1, pos2 - pos - 1);
			values[key] = val;
			offset = pos2 + 1;
		}
	}

	bool IsRedirectCode(int code)
	{
		return code >= 300 && code < 400;
	}

	bool Parse(const std::string& iurl, std::string& host, int& port, std::string& path, std::string& params)
	{
		char _url[4096] = "";
		strncpy(_url, iurl.c_str(), sizeof(_url));
		char* url = &_url[0];

		port = 80;
		host[0] = '\0';
		path[0] = '\0';
		params[0] = '\0';

		const int cst_state_http_header = 0;
		const int cst_state_http_host = 1;
		const int cst_state_http_port = 2;
		const int cst_state_http_path = 3;
		const int cst_state_http_para = 4;

		int state = cst_state_http_header;

		while (url != NULL && state <= cst_state_http_para)
		{
			switch (state)
			{
			case cst_state_http_header:
				{
					if (strstr(url, "http://") != NULL)
						url += 7;
					state++;
				}
				break;
			case cst_state_http_host:
				{
					char* hostend = strchr(url, ':');
					if (hostend != NULL)
					{
						*hostend = '\0';
						host = url;
						*hostend = ':';

						url = hostend + 1;
						state++;
						break;
					}

					hostend = strchr(url, '/');
					if (hostend == NULL)
					{
						host = url;
						url = hostend;
						state += 2;
					}
					else
					{
						*hostend = '\0';
						url = hostend;
						*hostend = '/';

						url = hostend;
						state += 2;
						break;
					}
				}
				break;
			case cst_state_http_port:
				{
					char* portend = strchr(url, '/');
					if (portend == NULL)
					{
						port = atoi(url);
						state++;
						url = portend;
					}
					else
					{
						*portend = '\0';
						port = atoi(url);
						*portend = '/';
						state++;
						url = portend;
					}
				}
				break;
			case cst_state_http_path:
				{
					char* pathend = strchr(url, '?');
					if (pathend == NULL)
					{
						path = url;
						state++;
						url = pathend;
					}
					else
					{
						*pathend = '\0';
						path = url;
						*pathend = '?';
						state++;
						url = pathend + 1;
					}
				}
				break;
			case cst_state_http_para:
				{
					params = url;
					++state;
				}
				break;
			}
		}
		return true;
	}
}
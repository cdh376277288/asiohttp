#pragma once
#include <map>
#include <string>

namespace Http
{
	extern std::string UrlEncode(const std::string& URL);
	extern std::string UrlDecode(const std::string& URL);
	extern void ParseQueryValues(const std::string& str, std::map<std::string, std::string>& values);

	extern std::string IntegerToString(int s);
	extern int StringToInteger(const std::string& s);
	extern int HexStringToInteger(const std::string& s);

	extern bool IsRedirectCode(int code);
	extern bool Parse(const std::string& iurl, std::string& host, int& port, std::string& path, std::string& params);
}
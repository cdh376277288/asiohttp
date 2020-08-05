/*
 * ��飺����Boost����http��Post/Get����
 * ���ߣ�cdh
 * Boost�汾��boost_1_73_0
 */
#pragma once
#ifndef __CDH_HTTP_BASE_H__
#define __CDH_HTTP_BASE_H__
#include <iostream>
namespace cdh
{
	class HttpBase
	{
	public:

		HttpBase();

		virtual ~HttpBase();

		/*
		 * ����Post����
		 */
		virtual int post(const std::string& url) = 0;

		/*
		 * ����get����
		 */
		virtual int get(const std::string& url) = 0;

		virtual std::string getResponse(void) = 0;

	protected:
		typedef int(*pBuildRequest)(const std::string&, const std::string&, std::ostream&);

		/*
		 * ����URL
		 * parseUrl
		 * url: ��������URL
		 * out_server: ��������
		 * out_port: �˿ں�
		 * out_path: ��������ҳ
		 */
		static int parseUrl(const std::string& url, std::string& out_server, std::string& out_port, std::string& out_path);

		/*
		 * ����Post����
		 * buildPostRequest
		 */
		static int buildPostRequest(const std::string& server, const std::string& path, std::ostream& out_request);

		/*
		 * ����Get����
		 * buildGetRequest
		 */
		static int buildGetRequest(const std::string& server, const std::string& path, std::ostream& out_request);
	};
}

#endif // __CDH_HTTP_BASE_H__

#pragma once
#include "../Common/Netlib.h"

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <iostream>

#ifdef _DEBUG

#pragma comment(lib, "cpprest_2_10d.lib")	 // windows only
#else
#pragma comment(lib, "cpprest_2_10.lib")	 // windows only
#endif

//utility;                    // Common utilities like string conversions
//web;                        // Common features like URIs.
//web::http;                  // Common HTTP functionality
//web::http::client;          // HTTP client features
//concurrency::streams;       // Asynchronous streams

BEGIN_NETLIB

class restsdkHttp
{
public:
	restsdkHttp(void);
	~restsdkHttp(void);

	void GetHttpTest();
	void GetHttpAsyncTest();
	void GetJsonTest();
	static bool GetJson(const std::string& url, std::string& responseData);

	static BOOL RequestHttp(IN const CSNet::WEBREQ_METHOD method, IN const std::string url, IN const std::string postdata, OUT std::string& recievedata, IN const int default_time_out_second = 10);
	static BOOL GetHttp(IN const CSNet::WEBREQ_METHOD method, IN const std::string url, OUT std::string& data, const web::http::uri::components::component = web::http::uri::components::component::full_uri);
	static BOOL PostHttp(IN const CSNet::WEBREQ_METHOD method, IN const std::string url, IN const std::string postdata, OUT std::string& recievedata, IN const int default_time_out_second = 10);
	static void FCM_PostHttps(IN const std::string url, IN const std::string senddata, OUT std::string& recievedata);
};

END_NETLIB
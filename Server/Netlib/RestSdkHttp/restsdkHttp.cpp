#include "../../Include/Netlib/RestSdkHttp/restsdkHttp.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"

NetLib::restsdkHttp::restsdkHttp(void)
{
}

NetLib::restsdkHttp::~restsdkHttp(void)
{
}

void NetLib::restsdkHttp::GetHttpTest()
{
	web::http::client::http_client client(U("http://en.cppreference.com/w/"));

	auto resp = client.request(U("GET")).get();

	web::http::status_code status = resp.status_code();
	utility::string_t content_type = resp.headers().content_type();
	resp.extract_string(true).get();

	resp.extract_string(true).then([](utility::string_t sBoby)
	{
		std::wcout << sBoby << std::endl;

	}
	).wait();
}

void NetLib::restsdkHttp::GetHttpAsyncTest()
{
	web::http::client::http_client client(U("http://en.cppreference.com/w/"));

	client.request(U("GET")).then([](web::http::http_response resp)
	{
		std::wcout << U("STATUS : ") << resp.status_code() << std::endl;
		std::wcout << "content-type : " << resp.headers().content_type() << std::endl;

		resp.extract_string(true).then([](utility::string_t sBoby)
		{
			std::wcout << sBoby << std::endl;

		}
		).wait();
	}).wait();
}

void NetLib::restsdkHttp::GetJsonTest()
{
	web::http::client::http_client client(U("http://date.jsontest.com/"));
	web::http::http_request req(web::http::methods::GET);

	client.request(req).then([=](web::http::http_response r)
	{
		std::wcout << U("STATUS : ") << r.status_code() << std::endl;
		std::wcout << "content-type : " << r.headers().content_type() << std::endl;
		//{
		//		"time": "11:25:23 AM",
		//		"milliseconds_since_epoch" : 1423999523092,
		//		"date" : "02-15-2015"
		//}

		r.extract_json(true).then([](web::json::value v)
		{
			std::wcout << v.at(U("date")).as_string() << std::endl;
			std::wcout << v.at(U("time")).as_string() << std::endl;
		}).wait();
	}).wait();
}

bool NetLib::restsdkHttp::GetJson(const std::string& url, std::string& responseData)
{
	try
	{
		web::http::client::http_client_config cfg; cfg.set_timeout(std::chrono::seconds(2));
		web::http::client::http_client client(utility::conversions::to_string_t(url), cfg);

		web::http::http_request request(web::http::methods::GET);
		request.headers().add(U("Accept"), U("application/json"));

		auto response = client.request(request).get();

		if (response.status_code() == web::http::status_codes::OK)
		{
			responseData = response.extract_utf8string().get();
			return true;
		}
		else
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, "GetJson Error: " + response.status_code());
			return false;
		}
	}
	catch (const std::exception& e)
	{
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, "GetJson Exception: %s", e.what());
		return false;
	}
}

BOOL NetLib::restsdkHttp::RequestHttp(IN const CSNet::WEBREQ_METHOD method, IN const std::string url, IN const std::string postdata, OUT std::string& recievedata, IN const int default_time_out_second)
{
	if(method == CSNet::WEBREQ_METHOD::GET)
		return GetHttp(method, url, recievedata);
	else
		return PostHttp(method, url, postdata, recievedata, default_time_out_second);
}

BOOL NetLib::restsdkHttp::GetHttp(IN const CSNet::WEBREQ_METHOD method, IN const std::string url, OUT std::string& data, web::http::uri::components::component encodingtype)
{
	try
	{
		// dead url
		//http_client client(U("http://10.253.28.132:11000/RequestPrivateServer.aspx?op=get_user_aid&platform_type=100&billing_type=1000&version=0&user_id=aa9244ec196b037b74e22e52af44d3edff0a6ac2eae6eac96bcdd1aaded73693"));
		// exception string http://211.253.28.132:11000/RequestPrivateServer.aspx?op=set_warpoint&aid=24&cid=15&stat={ACTIVE_SOUL_WAR_POINT:259567,ATTACK_MAX:2985,ATTACK_MIN:3415,BaseHP:13580,BaseMP:1000,CPR:0,CRITICAL:7.25,CRITICAL_PROTECTION:7.25,CRITICAL_RATING:1.6449999809265137,DEFENCE_CRITICAL_RATING:0.14499999582767487,DEFENCE_POINT:1692,DEFENCE_POWER:0,EXP:0,HP:166617,HPMax:166617,LEVEL:0,MAX_CRITICAL_PROP:100,MP:1966,MPMax:1966,PASSIVE_SOUL_WAR_POINT:36000,WAR_POINT:787656}
		// real url
		web::http::uri urlencoding;
		utility::string_t tUrl;
		tUrl.append(CA2W(url.c_str()));
		utility::string_t encodeduri = urlencoding.encode_uri(tUrl, encodingtype);

		// timeout 10段稽 実特
		web::http::client::http_client_config cfg; cfg.set_timeout(std::chrono::seconds(10));

		web::http::client::http_client client(encodeduri, cfg);

		auto resp = client.request(U("GET")).get();

		web::http::status_code status = resp.status_code();
		utility::string_t content_type = resp.headers().content_type();
		utility::string_t test = resp.extract_string(true).get();
		data = CW2A(test.c_str());

		return TRUE;
	}
	catch (web::uri_exception &e)
	{
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, "uri_exception ::GetHttp %s", e.what());
		return FALSE;
	}
	catch (const std::exception &e)
	{
		char szTest[CSDef::EDef::MAX_BUFFER_1024_LEN] = { 0, };
		const char* p = e.what();
		UTF8ToMultiByte(szTest, static_cast<size_t>(sizeof(szTest)), const_cast<char*>(p), strlen(p));
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, "std::exception restsdkHttp::GetHttp [ %s ]", szTest);
		return FALSE;
	}
	catch (...)
	{
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("known exception restsdkHttp::GetHttp"));
		return FALSE;
	}
}

BOOL NetLib::restsdkHttp::PostHttp(IN const CSNet::WEBREQ_METHOD method, IN const std::string url, IN const std::string postdata, OUT std::string& recievedata, IN const int default_time_out_second)
{
	try
	{
		//http_client client(U("http://10.253.28.132:11000/RequestPrivateServer.aspx?op=get_user_aid&platform_type=100&billing_type=1000&version=0&user_id=aa9244ec196b037b74e22e52af44d3edff0a6ac2eae6eac96bcdd1aaded73693"));
		web::uri urlencoding;
		utility::string_t tUrl;
		tUrl.append(CA2W(url.c_str()));
		utility::string_t encodeduri = urlencoding.encode_uri(tUrl, web::http::uri::components::full_uri);
		std::vector<unsigned char> body_data;
		std::copy(postdata.begin(), postdata.end(), std::back_inserter(body_data));

		// timeout 10段稽 実特
		web::http::client::http_client_config cfg;
		cfg.set_timeout(std::chrono::seconds(default_time_out_second));

		web::http::client::http_client client(encodeduri, cfg);
		web::http::http_request request(web::http::methods::POST);
		request.set_body(body_data);

		auto resp = client.request(request).get();

		web::http::status_code status = resp.status_code();
		utility::string_t content_type = resp.headers().content_type();
		//string_t test = resp.extract_string(true).get();
		std::vector<unsigned char> resp_data = resp.extract_vector().get();
		//recievedata = CW2A(test.c_str());

		std::copy(resp_data.begin(), resp_data.end(), std::back_inserter(recievedata));

		return TRUE;
	}
	catch (web::uri_exception &e)
	{
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, "uri_exception ::PostHttps %s", e.what());
		return FALSE;
	}
	catch (const std::exception &e)
	{
		char szTest[CSDef::EDef::MAX_BUFFER_1024_LEN] = { 0, };
		const char* p = e.what();
		UTF8ToMultiByte(szTest, static_cast<size_t>(sizeof(szTest)), const_cast<char*>(p), strlen(p));
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, "std::exception restsdkHttp::PostHttps [ %s ]", szTest);
		return FALSE;
	}
	catch (...)
	{
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("known exception restsdkHttp::PostHttps"));
		return FALSE;
	}
}

void NetLib::restsdkHttp::FCM_PostHttps(IN const std::string url, IN const std::string senddata, OUT std::string& recievedata)
{
	try
	{
		//http_client client(U("http://10.253.28.132:11000/RequestPrivateServer.aspx?op=get_user_aid&platform_type=100&billing_type=1000&version=0&user_id=aa9244ec196b037b74e22e52af44d3edff0a6ac2eae6eac96bcdd1aaded73693"));
		web::uri urlencoding;
		utility::string_t tUrl;
		tUrl.append(CA2W(url.c_str()));
		utility::string_t encodeduri = urlencoding.encode_uri(tUrl, web::http::uri::components::full_uri);

		// timeout 10段稽 実特
		web::http::client::http_client_config cfg; cfg.set_timeout(std::chrono::seconds(10));

		web::http::client::http_client client(encodeduri, cfg);

		web::http::http_request request(web::http::methods::POST);
		/*string addvalue = "key=" + FcmPushKey;
		string_t tAddvalue = CA2W(addvalue.c_str());
		request.headers().add(L"Authorization", tAddvalue);*/
		request.headers().add(L"Content-Type", L"application/json");
		request.set_body(senddata);

		auto resp = client.request(request).get();

		web::http::status_code status = resp.status_code();
		utility::string_t content_type = resp.headers().content_type();
		utility::string_t test = resp.extract_string(true).get();
		recievedata = CW2A(test.c_str());
	}
	catch(web::uri_exception &e)
	{
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("uri_exception ::PostHttps %s"), e.what());
		return;
	}
	catch(const std::exception &e)
	{
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("std::exception restsdkHttp::PostHttps %s"), e.what());
		return;
	}
	catch(...)
	{
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("known exception restsdkHttp::PostHttps"));
		return;
	}
}
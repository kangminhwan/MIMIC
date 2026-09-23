#pragma once
#include "TableServerHeader.h"


#include "../Include/Netlib/WinInet/cWinInet.h"
#include "../Include/Netlib/Alert/cInterfaceAlert.h"
#include "../Include/Netlib/RestSdkHttp/restsdkHttp.h"

#ifdef UNICODE
#define tregex	std::wregex
#else 
#define tregex	std::regex
#endif

class MadeSlackNotification : public NetLib::cInterfaceAlert
{
private:
	tstring MADE_web_hook_url;
	std::string MADE_web_hook_url2;
	std::string s_serverinfo = "";

public:
	bool SendAlert( const tstring& alertmessage ) override;
	bool SendDumpAlert( const tstring& alertmessage ) override;
	bool SendSlack( std::string message );
	void Init( const std::string serverinfo );

private:
	static std::wstring GetErrorString( DWORD errorCode );
	static bool isJSON( const tstring& jsonString );

public:
	MadeSlackNotification();
	~MadeSlackNotification();
};


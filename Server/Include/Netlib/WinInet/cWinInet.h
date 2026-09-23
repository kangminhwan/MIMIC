#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cWinInet
{
private:
	TCHAR m_strBrowserName[ CSDef::EDef::MAX_BUFFER_512_LEN ];
	TCHAR m_strDomain[ CSDef::EDef::MAX_BUFFER_512_LEN ];
	TCHAR m_strVerb[ CSDef::EDef::MAX_BUFFER_512_LEN ]; // Post or Get
	INTERNET_PORT m_dwhttpPort;

	tstring m_strHeader;
	tstring m_strObject;
	tstring m_strOptional;
	bool m_bUseHTTPS;

	TCHAR m_strReadBuffer[ WININET_READ_BUFFER_SIZE ];

public:
	static E_WIN_INET_ERROR WebRequest( IN const tstring& url , IN const tstring& sendData , OUT std::string& receivedData , CSNet::WEBREQ_METHOD method = CSNet::WEBREQ_METHOD::GET , tstring header = _T( "" ) );
	static std::basic_string<TCHAR> ReplaceDoubleQuotes( const std::basic_string<TCHAR>& input );

private:
	E_WIN_INET_ERROR SetURL( CSNet::WEBREQ_METHOD method , const TCHAR* URL , const TCHAR* header , const TCHAR* browser_name = _T( "cWinInet" ) );
	E_WIN_INET_ERROR HttpsPostRequest( const tstring& jsonString , std::string& receivedData );

private:
	cWinInet();
	~cWinInet();
};

END_NETLIB
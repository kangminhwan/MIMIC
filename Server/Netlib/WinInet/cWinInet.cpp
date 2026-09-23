#include "../../Include/Netlib/WinInet/cWinInet.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"

inline static bool IsUrlBreaker(TCHAR ch)
{
	return !(ch == NULL || ch == '/' || ch == ':');
}

inline static bool IsRequestBreaker(TCHAR ch)
{
	return !(ch == NULL || ch == '/');
}

NetLib::cWinInet::cWinInet()
{
	memset(m_strBrowserName, 0x00, sizeof(m_strBrowserName));
	memset(m_strDomain, 0x00, sizeof(m_strDomain));
	memset(m_strVerb, 0x00, sizeof(m_strVerb));
	memset(m_strReadBuffer, 0x00, sizeof(m_strReadBuffer));
}


NetLib::cWinInet::~cWinInet()
{
}

E_WIN_INET_ERROR NetLib::cWinInet::SetURL( CSNet::WEBREQ_METHOD method , const TCHAR* URL , const TCHAR* header , const TCHAR* browser_name )
{
	if(_tcslen(URL) == 0)
	{
		return E_WIN_INET_ERROR::E_WIN_INET_ERROR_URL_PARSING_EMPTY;
	}

	_tcscpy_s(m_strBrowserName, browser_name);

	m_dwhttpPort = INTERNET_DEFAULT_HTTP_PORT;

	LPCTSTR splitter = _tcsstr(URL, _T("://"));
	if(splitter == nullptr)
	{
		return E_WIN_INET_ERROR::E_WIN_INET_ERROR_URL_PARSING;
	}

	LPCTSTR ptr = splitter != nullptr ? splitter + 3 : URL;

	m_bUseHTTPS = _tcsnicmp(URL, _T("https://"), 8) == 0;
	m_dwhttpPort = m_bUseHTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

	tstring domain_string;
	tstring port_string;
	tstring object_string;

	char ch = *ptr++;
	
	while (IsUrlBreaker(ch))
	{
		domain_string.push_back(ch);
		ch = *ptr++;
	}
	_tcscpy_s(m_strDomain, domain_string.c_str());

	if(ch == ':')
	{
		ch = *ptr++;
		while (IsUrlBreaker(ch))
		{
			port_string.push_back(ch);
			ch = *ptr++;
		}
		m_dwhttpPort = _ttoi(port_string.c_str());
	}

	if(method == CSNet::WEBREQ_METHOD::GET)
	{
		if(ch == '/')
		{
			ch = *ptr++;
			while (IsUrlBreaker(ch))
			{
				m_strObject.push_back(ch);
				ch = *ptr++;
			}
		}

		_tcscpy_s(m_strVerb, _T("GET"));
	}
	else
	{
		if(ch == '/')
		{
			m_strObject.push_back(ch);
			ch = *ptr++;
			while (ch != '?')
			{
				m_strObject.push_back(ch);
				ch = *ptr++;
			}
		}

		if(ch == '?')
		{
			ch = *ptr++;
			while (IsRequestBreaker(ch))
			{
				m_strOptional.push_back(ch);
				ch = *ptr++;
			}
		}

		_tcscpy_s(m_strVerb, _T("POST"));

		if(header && _tcslen(header) > 0)
		{
			m_strHeader = header;
		}
		else
		{
			m_strHeader = _T("Content-Type: application/x-www-form-urlencoded");
		}

		if(m_dwhttpPort <= 0)
		{
			return E_WIN_INET_ERROR::E_WIN_INET_ERROR_URL_PARSING_PORT;
		}
	}

	return E_WIN_INET_ERROR::E_WIN_INET_ERROR_OK;
}

E_WIN_INET_ERROR NetLib::cWinInet::HttpsPostRequest( const tstring& jsonString , std::string& receivedData )
{
	receivedData.clear();

	E_WIN_INET_ERROR eResult = E_WIN_INET_ERROR::E_WIN_INET_ERROR_ERROR;

	HINTERNET hInet = NULL;
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;
	DWORD dwFlags = 0;
	TCHAR szLen[ MAX_PATH ] = { 0 };
	TCHAR szHeader[ 2048 ] = { 0 };
	DWORD dwReadSize = 0;
	DWORD dwTotalData = 0;
	int byteLength = 0;
	char* utf8String = nullptr;
	BOOL bSendRequest = FALSE;

	NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();

	hInet = InternetOpenA( "cWinINet" , INTERNET_OPEN_TYPE_PRECONFIG , NULL , NULL , 0 );
	if ( !hInet ) {
		eResult = E_WIN_INET_ERROR::E_WIN_INET_ERROR_INTERNET_SESSION;
		if ( pLogQueue ) {
			pLogQueue->PushCommand( LOG_GRADE::LOG_CRI ,
									"PostRequest() InternetConnect Failed. Result:E_WIN_INET_ERROR_INTERNET_SESSION GLE:%u" ,
									GetLastError() );
		}
		goto error;
	}

	hConnect = InternetConnect( hInet , m_strDomain , m_dwhttpPort , _T("") , _T("") , INTERNET_SERVICE_HTTP , 0 , 0 );
	if ( !hConnect ) {
		eResult = E_WIN_INET_ERROR::E_WIN_INET_ERROR_INTERNET_CONNECT;
		if ( pLogQueue ) {
			pLogQueue->PushCommand( LOG_GRADE::LOG_CRI ,
				"PostRequest() InternetConnect Failed. Result:E_WIN_INET_ERROR_INTERNET_CONNECT GLE:%u Host:%s:%d" ,
				GetLastError() ,
				m_strDomain ,
				m_dwhttpPort );
		}
		goto error;
	}

	hRequest = HttpOpenRequest( hConnect , m_strVerb , m_strObject.c_str() , HTTP_VERSION , _T("") , NULL ,
								INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID ,
								0 );
	if ( !hRequest ) {
		eResult = E_WIN_INET_ERROR::E_WIN_INET_ERROR_INTERNET_OPEN_REQUEST;
		if ( pLogQueue ) {
			pLogQueue->PushCommand( LOG_GRADE::LOG_CRI ,
									"PostRequest() HttpOpenRequest Failed. Result:E_WIN_INET_ERROR_INTERNET_OPEN_REQUEST GLE:%u URL:%s" ,
									GetLastError() ,
									m_strObject.c_str() );
		}
		goto error;
	}

	
	dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
		SECURITY_FLAG_IGNORE_REVOCATION |
		SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTP |
		SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTPS |
		SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
		SECURITY_FLAG_IGNORE_CERT_CN_INVALID;

	InternetSetOption( hRequest , INTERNET_OPTION_SECURITY_FLAGS , &dwFlags , sizeof( dwFlags ) );

	
	

	_stprintf_s( szLen , _countof( szLen ) , _T( "%d" ) , jsonString.length() );

	// WideCharToMultiByte를 사용하여 문자열을 변환
	byteLength = WideCharToMultiByte( CP_UTF8 , 0 , jsonString.c_str() , -1 , NULL , 0 , NULL , NULL );
	utf8String = new char[ byteLength ];
	WideCharToMultiByte( CP_UTF8 , 0 , jsonString.c_str() , -1 , utf8String , byteLength , NULL , NULL );

	_stprintf_s( szHeader , _countof( szHeader ) ,
		_T( "Accept: */*\r\n" )
		_T( "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0;* Windows NT)\r\n" )
		_T( "Content-type: application/x-www-form-urlencoded\r\n" )
		_T( "Content-Type: application/json\r\n" )
		_T( "Content-length: %s\r\n\n" ) , szLen );

	HttpAddRequestHeaders( hRequest , szHeader , -1L , HTTP_ADDREQ_FLAG_ADD );

	// 바이트 수로 변환된 데이터 전송
	bSendRequest = HttpSendRequest( hRequest , NULL , 0 , ( LPVOID ) utf8String , byteLength - 1 );  // NULL 종료 문자 제외

	
	

	do {
		dwReadSize = 0;
		memset( m_strReadBuffer , 0x00 , sizeof( m_strReadBuffer ) );
		if ( !::InternetReadFile( hRequest , m_strReadBuffer , WININET_READ_BUFFER_SIZE - 1 , &dwReadSize ) ) {
			eResult = E_WIN_INET_ERROR::E_WIN_INET_ERROR_INTERNET_READ;
			if ( pLogQueue ) {
				pLogQueue->PushCommand( LOG_GRADE::LOG_CRI ,
										"CommunicateWinInet_POST() InternetReadFile Failed. Result:E_WIN_INET_ERROR_INTERNET_READ GLE:%u" ,
										GetLastError() );
			}
			goto error;
		}

		if ( dwReadSize == 0 )
			break;

		receivedData += ( char* ) m_strReadBuffer;
		dwTotalData += dwReadSize;

	} while ( dwReadSize > 0 );

	if ( receivedData.length() == 0 ) {
		eResult = E_WIN_INET_ERROR::E_WIN_INET_ERROR_NO_DATA;
		goto error;
	}

error:
	delete[] utf8String;
	if ( hRequest ) InternetCloseHandle( hRequest );
	if ( hConnect ) InternetCloseHandle( hConnect );
	if ( hInet ) InternetCloseHandle( hInet );

	return eResult;
}

E_WIN_INET_ERROR NetLib::cWinInet::WebRequest( IN const tstring& url , IN const tstring& sendData , OUT std::string& receivedData , CSNet::WEBREQ_METHOD method , tstring header )
{
	if(url.length() == 0)
	{
		return E_WIN_INET_ERROR::E_WIN_INET_ERROR_ERROR;
	}

	NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();

	NetLib::cWinInet http;
	E_WIN_INET_ERROR eResultSetURL = http.SetURL(method, url.c_str(), header.length() == 0 ? NULL : header.c_str());
	if(eResultSetURL != E_WIN_INET_ERROR::E_WIN_INET_ERROR_OK)
	{
		tstring replacedURL(url);
		ReplaceString(replacedURL, _T("%"), _T("*"));
		
		if(pLogQueue)
		{
			pLogQueue->PushCommand(	LOG_GRADE::LOG_CRI, 
									"WebRequest SetURL Error Result:%d Method:%d URL:%s",
									static_cast<int>(eResultSetURL),
									method,
									replacedURL.c_str());
		}
		return eResultSetURL;
	}

	E_WIN_INET_ERROR eReultWinInet = E_WIN_INET_ERROR::E_WIN_INET_ERROR_ERROR;

	__int64 startTick = ::GetTickCount64();
	if(method == CSNet::WEBREQ_METHOD::POST )
		eReultWinInet = http.HttpsPostRequest( sendData , receivedData );

	__int64 elapsTick = ::GetTickCount64() - startTick;
	if(pLogQueue && elapsTick >= 1000) {
		tstring replacedURL(url);
		ReplaceString(replacedURL, _T("%"), _T("*"));
		pLogQueue->PushCommand(	LOG_GRADE::LOG_CRI,
								"WebRequest() Response Over 1 second!! elapsTime[%I64d ms] Result:%d Method:%d URL:%s",
								elapsTick,
								static_cast<int>(eReultWinInet),
								method,
								replacedURL.c_str());
	}

	return eReultWinInet;
}

// tstring에서 모든 "를 \"로 대체하는 함수
std::basic_string<TCHAR> NetLib::cWinInet::ReplaceDoubleQuotes( const std::basic_string<TCHAR>& input ) {
	std::basic_string<TCHAR> result;
	for ( TCHAR ch : input ) {
		if ( ch == _T( '\"' ) ) {
			result += _T( "\\\"" );
		}
		else {
			result += ch;
		}
	}
	return result;
}
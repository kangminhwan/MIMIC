#include "LocalSettings.h"
#include "MadeSlackNotification.h"

#include <regex>


#ifdef UNICODE
#define tregex	std::wregex
#else 
#define tstring	std::regex
#endif

/*
#사내-쉼터
[local webhook removed]

#test-noti
[local webhook removed]
*/

MadeSlackNotification::MadeSlackNotification()
{
    //MADE_web_hook_url = _T("[local webhook removed]");
    MADE_web_hook_url = _T(MIMIC_SLACK_ALERT_URL);
    //MADE_web_hook_url2 = "[local webhook removed]";
    MADE_web_hook_url2 = MIMIC_SLACK_NOTIFICATION_URL;

}
MadeSlackNotification::~MadeSlackNotification()
{
    if( s_serverinfo!="" )
    SendSlack( s_serverinfo + " END" );
}

std::wstring MadeSlackNotification::GetErrorString( DWORD errorCode ) {
    LPTSTR lpMsgBuf = NULL;
    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS ,
        NULL ,
        errorCode ,
        0 , // Default language 
        ( LPTSTR ) &lpMsgBuf ,
        0 ,
        NULL );

    std::wstring errorMessage( static_cast< wchar_t* >( lpMsgBuf ) );
    LocalFree( lpMsgBuf );
    return errorMessage;
}
bool MadeSlackNotification::SendSlack( std::string alertmessage )
{
    if (MADE_web_hook_url.empty() || MADE_web_hook_url2.empty()) return false;
    int allowedMessageSize = 1900;
    if ( alertmessage.length() > allowedMessageSize )
        return false;
    std::string message = std::format( R"({{"text":"{}"}})" , alertmessage );
    std::string receivedData;
    if ( NetLib::restsdkHttp::RequestHttp( CSNet::WEBREQ_METHOD::POST , MADE_web_hook_url2 , message , receivedData , 1 ) == FALSE )
    {
        return Server::ServiceStatusCode::ServiceStatus_RuntimeFault;
    }
}
void MadeSlackNotification::Init( const std::string serverinfo )
{
    s_serverinfo = serverinfo;
    SendSlack( serverinfo+" START");
}
bool MadeSlackNotification::SendAlert( const tstring& alertmessage )
{
    // json 형식 인지 확인 하지 않는다.
    //if ( false == isJSON( alertmessage ) )
    //    return false;

    if (MADE_web_hook_url.empty() || MADE_web_hook_url2.empty()) return false;
    int allowedMessageSize = 1900;
    if ( alertmessage.length() > allowedMessageSize )
        return false;

    //tstring replaceString = alertmessage;

    //ReplaceString( replaceString , _T("\""), _T(""));
    //ReplaceString( replaceString , _T( "{" ) , _T( "" ) );
    //ReplaceString( replaceString , _T( "}" ) , _T( "" ) );

    //// 슬랙 웹훅은 아래와 같은 형식으로 전송이 되어야 한다.
    //// {"text": "Hello, World!"}
    //TCHAR buffer[ 2048 ] = { 0, };
    //_stprintf_s( buffer , _countof( buffer ), _T( "{\"text\" : \"%s\"}") , replaceString.c_str() );

	tstring header = _T("application/json");
	//std::string t_data = "{\"text\":\"test\"}";

    //std::basic_string<TCHAR> jsonString = alertmessage;
    //tstring newMessage = NetLib::cWinInet::ReplaceDoubleQuotes( alertmessage );

    /*std::string receivedData;
    if ( NetLib::restsdkHttp::RequestHttp( CSNet::WEBREQ_METHOD::POST , MADE_web_hook_url2 , alertmessage , receivedData , 1 ) == FALSE )
    {
        return Server::ServiceStatusCode::ServiceStatus_RuntimeFault;
    }*/

	//NetLib::cWinInet::WebRequest( MADE_web_hook_url , buffer , receivedData, CSNet::WEBREQ_METHOD::POST , header.c_str() );
    return true;
}

bool MadeSlackNotification::SendDumpAlert( const tstring& alertmessage )
{
    // json 형식 인지 확인 하지 않는다.
    //if ( false == isJSON( alertmessage ) )
    //    return false;

    if (MADE_web_hook_url.empty() || MADE_web_hook_url2.empty()) return false;
    int allowedMessageSize = 1900;
    if ( alertmessage.length() > allowedMessageSize )
        return false;

    tstring replaceString = alertmessage;

    ReplaceString( replaceString , _T( "\"" ) , _T( "" ) );
    ReplaceString( replaceString , _T( "{" ) , _T( "" ) );
    ReplaceString( replaceString , _T( "}" ) , _T( "" ) );

    // 슬랙 웹훅은 아래와 같은 형식으로 전송이 되어야 한다.
    // {"text": "Hello, World!"}
    TCHAR buffer[ 2048 ] = { 0, };
    _stprintf_s( buffer , _countof( buffer ) , _T( "{\"text\" : \"%s\"}" ) , replaceString.c_str() );

    tstring header = _T( "application/json" );
    std::string recievedata;

    //std::basic_string<TCHAR> jsonString = alertmessage;
    //tstring newMessage = NetLib::cWinInet::ReplaceDoubleQuotes( alertmessage );

    std::string receivedData;
    NetLib::cWinInet::WebRequest( MADE_web_hook_url , buffer , receivedData , CSNet::WEBREQ_METHOD::POST , header.c_str() );
    return true;
}

bool MadeSlackNotification::isJSON( const tstring& jsonString ) {
    // 간단한 JSON 형식 유효성을 검사하는 정규 표현식
    tregex jsonPattern( _T( R"(\{\s*\"(?:[^\"]*\"\:\s*(?:\"[^\"]*\"\s*\,?\s*|\d*\.\d*\s*\,?\s*|\d*\s*\,?\s*)\s*)*\})" ) );

    // 정규 표현식과 매치되는지 확인
    return std::regex_match( jsonString , jsonPattern );
}
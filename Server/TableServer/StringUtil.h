#pragma once

#include <codecvt>
#include <locale>
#include <string>
#include <cwctype>
#include <Windows.h>
#include <string>
#include <regex>
#include <vector>
#include <json/json.h>
#include <nlohmann/json.hpp> // nlohmann/json 라이브러리 사용

using json = nlohmann::json;
class StringUtil
{
public:
   /* static std::wstring utf8_to_wstring( const std::string& utf8_str )
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes( utf8_str );
    }*/

    static std::wstring Utf8ToWide( const std::string& utf8Str )
    {
        int wideSize = MultiByteToWideChar( CP_UTF8 , 0 , utf8Str.c_str() , -1 , nullptr , 0 );
        if ( wideSize == 0 )
        {
            // 오류 처리
            return L"";
        }

        std::wstring wideStr;
        wideStr.resize( wideSize );
        MultiByteToWideChar( CP_UTF8 , 0 , utf8Str.c_str() , -1 , &wideStr[ 0 ] , wideSize );
        return wideStr;
    }

    static std::string WideToUtf8( const std::wstring& wideStr )
    {
        int utf8Size = WideCharToMultiByte( CP_UTF8 , 0 , wideStr.c_str() , -1 , nullptr , 0 , nullptr , nullptr );
        if ( utf8Size == 0 )
        {
            // 오류 처리
            return "";
        }

        std::string utf8Str;
        utf8Str.resize( utf8Size );
        WideCharToMultiByte( CP_UTF8 , 0 , wideStr.c_str() , -1 , &utf8Str[ 0 ] , utf8Size , nullptr , nullptr );
        return utf8Str;
    }

    static std::u8string WstringToUtf8( const std::wstring& wideStr )
    {
        int utf8Size = WideCharToMultiByte( CP_UTF8 , 0 , wideStr.c_str() , -1 , nullptr , 0 , nullptr , nullptr );
        if ( utf8Size == 0 )
        {
            // 오류 처리
            return u8"";
        }

        std::vector<char> buffer( utf8Size );
        WideCharToMultiByte( CP_UTF8 , 0 , wideStr.c_str() , -1 , buffer.data() , utf8Size , nullptr , nullptr );
        return std::u8string( buffer.begin() , buffer.end() );
    }

    static std::wstring ConvertToWide( const std::string& utf8Str ) {
        int wideStrLen = MultiByteToWideChar( CP_UTF8 , 0 , utf8Str.c_str() , -1 , NULL , 0 );
        if ( wideStrLen == 0 ) {
            // Error handling
            return L"";
        }

        std::wstring wideStr;
        wideStr.resize( wideStrLen );
        if ( MultiByteToWideChar( CP_UTF8 , 0 , utf8Str.c_str() , -1 , &wideStr[ 0 ] , wideStrLen ) == 0 ) {
            // Error handling
            return L"";
        }

        return wideStr;
    }

    static std::string ConvertToUtf8( const std::wstring& wideStr ) {
        int utf8StrLen = WideCharToMultiByte( CP_UTF8 , 0 , wideStr.c_str() , -1 , NULL , 0 , NULL , NULL );
        if ( utf8StrLen == 0 ) {
            // Error handling
            return "";
        }

        std::string utf8Str;
        utf8Str.resize( utf8StrLen );
        if ( WideCharToMultiByte( CP_UTF8 , 0 , wideStr.c_str() , -1 , &utf8Str[ 0 ] , utf8StrLen , NULL , NULL ) == 0 ) {
            // Error handling
            return "";
        }

        return utf8Str;
    }

    static bool isValidString( const std::string& nickname )
    {
        try {
            // UTF-8 문자열을 wstring으로 변환
            std::wstring w_nickname = Utf8ToWide( nickname );
            if ( w_nickname.empty() && !nickname.empty() ) {
                // 변환 실패 시 false 반환
                return false;
            }
            
            // null terminator와 제어 문자들 제거
            std::wstring cleaned_nickname;
            for ( wchar_t wch : w_nickname ) {
                // null terminator, 제어 문자, 보이지 않는 문자들 제거
                if ( wch != L'\0' && 
                     !std::iswcntrl( wch ) && 
                     wch != 0x200B &&  // Zero Width Space
                     wch != 0x200C &&  // Zero Width Non-Joiner
                     wch != 0x200D &&  // Zero Width Joiner
                     wch != 0xFEFF ) { // Byte Order Mark
                    cleaned_nickname += wch;
                }
            }
            
            // 정리된 문자열이 비어있다면 false
            if ( cleaned_nickname.empty() ) {
                return false;
            }
            
            // wstring 기반 정규식: 한글(가-힣) + 영문 대소문자 + 숫자만 허용
            // ^[가-힣a-zA-Z0-9]+$  → 전체가 이 문자들로만 구성되어야 함
            std::wregex pattern( L"^[가-힣a-zA-Z0-9]+$" );

            return std::regex_match( cleaned_nickname , pattern );
        }
        catch ( const std::exception& e ) {
            // 정규식 처리 중 오류 발생 시 false 반환
            return false;
        }
    }
    static std::vector<uint64> ParseUInt64Array( const std::wstring& valueText )
    {
        std::vector<uint64> result;

        size_t start = 0;

        while ( start < valueText.size() )
        {
            size_t pos = valueText.find( L'/' , start );

            std::wstring token;
            if ( pos == std::wstring::npos )
                token = valueText.substr( start );
            else
                token = valueText.substr( start , pos - start );

            if ( !token.empty() )
            {
                wchar_t* endPtr = nullptr;
                uint64 value = std::wcstoull( token.c_str() , &endPtr , 10 );

                if ( endPtr == nullptr || *endPtr != L'\0' )
                {
                    // 로그 남기고 실패 처리
                    TraceA( "ParseUInt64Array failed" );
                    return {};
                }

                result.push_back( value );
            }

            if ( pos == std::wstring::npos )
                break;

            start = pos + 1;
        }

        return result;
    }
    //// 디버깅용: 문자열의 각 문자 코드를 확인하는 함수
    //static std::string debugStringCharCodes( const std::string& str ) {
    //    std::string result = "Debug string analysis:\n";
    //    std::wstring w_str = Utf8ToWide( str );
    //    
    //    result += "Original string: \"" + str + "\"\n";
    //    result += "Length (bytes): " + std::to_string( str.length() ) + "\n";
    //    result += "Length (wide chars): " + std::to_string( w_str.length() ) + "\n";
    //    result += "Character codes: ";
    //    
    //    for ( size_t i = 0; i < w_str.length(); ++i ) {
    //        wchar_t wch = w_str[i];
    //        result += "U+" + std::to_string( static_cast<unsigned int>( wch ) ) + " ";
    //        
    //        if ( wch == L'\0' ) result += "(NULL) ";
    //        else if ( std::iswcntrl( wch ) ) result += "(CTRL) ";
    //        else if ( wch == 0x200B ) result += "(ZWSP) ";
    //        else if ( wch == 0x200C ) result += "(ZWNJ) ";
    //        else if ( wch == 0x200D ) result += "(ZWJ) ";
    //        else if ( wch == 0xFEFF ) result += "(BOM) ";
    //    }
    //    
    //    return result;
    //}

    // SQL 인젝션 방지를 위한 검증 함수들
    static bool isSqlSafe( const std::string& input )
    {
        // SQL 인젝션 위험 문자 검사
        // 따옴표, 세미콜론, 주석, 연산자 등 차단
        std::regex dangerousPattern( R"(['"`;#\-\*\+\=\<\>\!\(\)\[\]\{\}\\&\$@\|\/\%\s])" );
        return !std::regex_search( input , dangerousPattern );
    }

    static bool isValidNickname( const std::string& nickname )
    {
        // 닉네임 전용 검증 (더 엄격)
        if ( nickname.empty() || nickname.length() > 64 ) {
            return false;
        }
        
        // UTF-8 문자 수 검증 (최대 20자)
        std::wstring w_nick = Utf8ToWide( nickname );
        if ( w_nick.length() > 20 ) {
            return false;
        }
        
        // 정규식 검증
        return isValidString( nickname );
    }

    static std::string sanitizeForSql( const std::string& input )
    {
        // SQL 안전한 문자열로 변환 (영문, 숫자만)
        std::string result;
        for ( char ch : input ) {
            if ( std::isalnum( static_cast< unsigned char >( ch ) ) ) {
                result += ch;
            }
        }
        return result;
    }

    static std::string sanitizeForLog( const std::string& input )
    {
        // 로그 안전한 문자열로 변환 (기존 함수 별칭)
        return RemoveSpecialCharactersAndSpaces( input );
    }

    static std::string ConvertToString( const std::basic_string<TCHAR>& tstr )
    {
#ifdef UNICODE
        // UNICODE인 경우: std::wstring을 std::string으로 변환
        std::wstring wstr( tstr );
        int size_needed = WideCharToMultiByte( CP_UTF8 , 0 , &wstr[ 0 ] , ( int ) wstr.size() , NULL , 0 , NULL , NULL );
        std::string strTo( size_needed , 0 );
        WideCharToMultiByte( CP_UTF8 , 0 , &wstr[ 0 ] , ( int ) wstr.size() , &strTo[ 0 ] , size_needed , NULL , NULL );
        return strTo;
#else
        // 멀티바이트인 경우: std::string으로 바로 변환
        return std::string( tstr.begin() , tstr.end() );
#endif
    }


    // 문자열에서 공백 제거하는 함수
    static void RemoveSpaces( std::string& str )
    {
        size_t pos;
        while ( ( pos = str.find( ' ' ) ) != std::string::npos ) {
            str.erase( pos , 1 );
        }
    }

    // 보이지 않는 공백문자 제거
    static std::string RemoveInvisibleSpaces( const std::string& str ) {
        std::string result;
        std::copy_if( str.begin() , str.end() , std::back_inserter( result ) , []( char c ) { return !std::isspace( static_cast< unsigned char >( c ) ); } );
        return result;
    }

    static bool ContainsInvisibleSpaces( const std::string& str ) {
        for ( char ch : str ) {
            if ( ch != ' ' && std::isspace( static_cast< unsigned char >( ch ) ) ) {
                return true;
            }
        }
        return false;
    }

    static std::wstring RemoveZeroWhiteSpace( std::wstring& str)
    {
        //std::wstring test = L"한글인가​"; // Zero Width Space가 포함된 문자열
        std::wstring filtered;

        // Zero Width Space를 제거하고 필터된 문자열을 생성
        for ( wchar_t c : str ) {
            if ( c != 8203 ) { // Zero Width Space를 제외한 문자만 필터링
                filtered += c;
            }
        }
        return filtered;
    }

    static void RemoveZeroWidthSpace( std::string& str )
    {
        size_t pos;
        while ( ( pos = str.find( '\u200B' ) ) != std::string::npos ) {
            str.erase( pos , 1 );
        }
    }

    // 특문, 공백 제거 함수
    static std::string RemoveSpecialCharactersAndSpaces( const std::string& input ) {
        std::string result;
        for ( char ch : input ) {
            if ( std::isalnum( static_cast< unsigned char >( ch ) ) ) {
                result += ch;
            }
        }
        return result;
    }

    static bool HasPercent( const std::string& input ) {
        std::string result;
        result.reserve( input.size() );

        for ( size_t i = 0; i < input.size(); ++i ) {
            if ( input[ i ] == '%' )
                return true;
        }

        return false;
    }

    //static std::string JsonToString( const Json::Value& jsonValue ) {
    //    return jsonValue.asString();
    //    Json::StreamWriterBuilder writer;
    //    writer[ "indentation" ] = ""; // 한 줄 출력
    //    return Json::writeString( writer , jsonValue );
    //}

    static std::string JsonToString( const json& jsonValue ) {
        // json 객체를 문자열로 변환 (예쁘게 출력하기 위해 들여쓰기 4칸 사용)
        return jsonValue.dump( 4 );
    }

    static json StringToJson( const std::string& jsonString ) {
        try {
            // 문자열을 json 객체로 파싱
            return json::parse( jsonString );
        }
        catch ( const std::exception& e ) {
            // 파싱 중 오류가 발생하면 빈 json 객체 반환
            std::cerr << "JSON 파싱 오류: " << e.what() << std::endl;
            return json(); // 빈 JSON 객체 반환
        }
    }

    // 디시리얼라이즈: std::string -> Json::Value
   /* static Json::Value StringToJson( const std::string& jsonString ) {
        Json::CharReaderBuilder reader;
        Json::Value jsonValue;
        std::string errors;

        std::istringstream iss( jsonString );

        return jsonValue;
    }*/

    template <typename T>
    static std::string JoinGeneric( const std::vector<T>& vec , const std::string& delim )
    {
        std::ostringstream oss;
        for ( size_t i = 0; i < vec.size(); ++i ) {
            if ( i != 0 ) oss << delim;
            oss << vec[ i ];  // T가 ostream 지원해야 함
        }
        return oss.str();
    }

    static std::vector<std::string> SplitGeneric( const std::string& str , const std::string& delim )
    {
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = 0;

        while ( ( end = str.find( delim , start ) ) != std::string::npos ) {
            tokens.emplace_back( str.substr( start , end - start ) );
            start = end + delim.length();
        }

        tokens.emplace_back( str.substr( start ) ); // 마지막 토큰

        return tokens;
    }
};
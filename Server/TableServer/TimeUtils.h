#pragma once
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <mysql.h>

class TimeUtils
{
public:
    static std::string GetCurrentDateTime() {
        // 현재 시간 구하기
        std::time_t now = std::time(nullptr);

        // 시간 구조체 생성
        std::tm timeInfo;

        // 시간 구조체를 현재 시간으로 초기화
        localtime_s(&timeInfo, &now); // Windows에서는 localtime_s 함수를 사용합니다.

        // 원하는 형식으로 문자열로 변환
        char buffer[20]; // "YYYY-MM-DD HH:MM:SS"까지의 문자열은 19자리입니다.
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);

        return std::string(buffer);
    }

    static bool IsMonthsApart( std::time_t time1 , std::time_t time2 , int month ) {
        std::tm tm1 = {};
        std::tm tm2 = {};

        localtime_s( &tm1 , &time1 );
        localtime_s( &tm2 , &time2 );

        // 연, 월, 일 차이 계산
        int year_diff = tm2.tm_year - tm1.tm_year;
        int month_diff = tm2.tm_mon - tm1.tm_mon;
        int day_diff = tm2.tm_mday - tm1.tm_mday;

        int total_month_diff = year_diff * 12 + month_diff;

        // 만약 일(day)이 부족하면 한 달 미만으로 간주해야 함
        if ( day_diff < 0 )
            total_month_diff--;

        return total_month_diff >= month;
    }
    static std::string MYSQLTimeToString(const MYSQL_TIME& time , bool bNeedsTime = true ) {
        char buffer[20]; // 충분한 크기의 문자열 버퍼

        if( bNeedsTime )
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
            time.year, time.month, time.day, time.hour, time.minute, time.second);
        else
            snprintf( buffer , sizeof( buffer ) , "%04d-%02d-%02d" ,
            time.year , time.month , time.day );

        return std::string(buffer);
    }

    static ATL::CString currentDateTime() {
        time_t now = time( 0 );
        tm tstruct;
        localtime_s( &tstruct , &now );
        TCHAR buf[ 80 ] = { 0, };
        _tcsftime( buf , sizeof( buf ) , _T( "%Y-%m-%d.%X" ) , &tstruct );

        return buf;
    }

    static std::string GetCurrentKSTDateTimeString( int hoursToAdd = 0 ) {
        // Get current system time in KST timezone
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t( now );

        // Convert to KST timezone
        std::tm kst_tm;
        localtime_s( &kst_tm , &now_time );

        // Add hours
        kst_tm.tm_hour += hoursToAdd;

        // Normalize tm struct
        std::mktime( &kst_tm );

        // Format the date and time string
        std::ostringstream oss;
        oss << std::put_time( &kst_tm , "%Y-%m-%d %H:%M:%S" );
        return oss.str();
    }

    static std::string GetCurrentKSTDateTimeString_AddMinutes( int minutesToAdd = 0 ) {
        // Get current system time in KST timezone
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t( now );

        // Convert to KST timezone
        std::tm kst_tm;
        localtime_s( &kst_tm , &now_time );

        // Add minutes
        kst_tm.tm_min += minutesToAdd;

        // Normalize tm struct
        std::mktime( &kst_tm );

        // Format date/time
        std::ostringstream oss;
        oss << std::put_time( &kst_tm , "%Y-%m-%d %H:%M:%S" );
        return oss.str();
    }

    static std::string GetTimeOneHourLater() {
        // 현재 시간 구하기
        std::time_t now = std::time( nullptr );

        // 한 시간을 더함 (60초 * 60분 = 3600초)
        now += 3600;

        // 시간 구조체 생성
        std::tm timeInfo;

        // 시간 구조체를 한 시간 뒤로 초기화
        localtime_s( &timeInfo , &now ); // Windows에서는 localtime_s 함수를 사용합니다.

        // 원하는 형식으로 문자열로 변환
        char buffer[ 20 ]; // "YYYY-MM-DD HH:MM:SS"까지의 문자열은 19자리입니다.
        std::strftime( buffer , sizeof( buffer ) , "%Y-%m-%d %H:%M:%S" , &timeInfo );

        return std::string( buffer );
    }

    static std::tm MysqlTimeToTM( const MYSQL_TIME& mysql_time ) {
        std::tm time_info;
        time_info.tm_year = mysql_time.year - 1900; // Years since 1900
        time_info.tm_mon = mysql_time.month - 1;    // Months since January (0-based)
        time_info.tm_mday = mysql_time.day;         // Day of the month (1-31)
        time_info.tm_hour = mysql_time.hour;        // Hours since midnight (0-23)
        time_info.tm_min = mysql_time.minute;       // Minutes after the hour (0-59)
        time_info.tm_sec = mysql_time.second;       // Seconds after the minute (0-59)
        time_info.tm_isdst = -1; // Daylight Saving Time flag (not set)
        return time_info;
    }

    // std::string test = "2024-04-06 12:22:37";
    static std::time_t StringToTimeTM( const std::string time_string ) {

        // 문자열을 시간으로 변환
        std::tm tm = {};
        std::istringstream ss( time_string );
        ss >> std::get_time( &tm , "%Y-%m-%d %H:%M:%S" );
        std::time_t t = std::mktime( &tm );
        return t;
    }

    static std::time_t TodayStartTimeTM()
    {
        // 현재 시간 구하기
        std::time_t now = std::time( nullptr );
        std::tm tm_now;
        localtime_s( &tm_now , &now );

        // 오늘 00시의 시간을 구하기
        tm_now.tm_hour = 0;
        tm_now.tm_min = 0;
        tm_now.tm_sec = 0;
        std::time_t today_midnight = std::mktime( &tm_now );
        return today_midnight;
    }

    static std::time_t SelectTimeStartTimeTM( const std::time_t _time)
    {
        // 현재 시간 구하기
        std::tm tm_now;
        localtime_s( &tm_now , &_time );

        // 오늘 00시의 시간을 구하기
        tm_now.tm_hour = 0;
        tm_now.tm_min = 0;
        tm_now.tm_sec = 0;
        std::time_t today_midnight = std::mktime( &tm_now );
        return today_midnight;
    }

    static std::string NextMonthStartTimeString()
    {
        // 현재 시간 구하기
        std::time_t now = std::time( nullptr );
        std::tm tm_now;
        localtime_s( &tm_now , &now );

        // 다음 달 1일 0시의 시간 구하기
        tm_now.tm_hour = 0;
        tm_now.tm_min = 0;
        tm_now.tm_sec = 0;
        tm_now.tm_mon += 1;
        if ( tm_now.tm_mon == 12 ) {
            tm_now.tm_year += 1;
            tm_now.tm_mon = 0; // 다음 년도의 1월로 설정
        }
        tm_now.tm_mday = 1; // 다음 달의 1일로 설정

        std::time_t next_month_start = std::mktime( &tm_now );

        // 다음 달 1일 0시의 시간을 문자열로 변환
        std::tm tm_next_month_start;
        localtime_s( &tm_next_month_start , &next_month_start );

        std::stringstream ss;
        ss << std::put_time( &tm_next_month_start , "%Y-%m-%d %H:%M:%S" );
        return ss.str();
    }

    // 이번달의 1일 0시 기준시를 구한다.
    static std::string ThisMonthStartTimeString()
    {
        // 현재 시간 구하기
        std::time_t now = std::time( nullptr );
        std::tm tm_now;
        localtime_s( &tm_now , &now );

        // 이번 달 1일 0시의 시간 구하기
        tm_now.tm_hour = 0;
        tm_now.tm_min = 0;
        tm_now.tm_sec = 0;
        tm_now.tm_mday = 1; // 이번 달의 1일로 설정

        std::time_t this_month_start = std::mktime( &tm_now );

        // 이번 달 1일 0시의 시간을 문자열로 변환
        std::tm tm_this_month_start;
        localtime_s( &tm_this_month_start , &this_month_start );

        std::stringstream ss;
        ss << std::put_time( &tm_this_month_start , "%Y-%m-%d %H:%M:%S" );
        return ss.str();
    }

    static std::time_t NextMonthStartTimeTM()
    {
        // 현재 시간 구하기
        std::time_t now = std::time( nullptr );
        std::tm tm_now;
        localtime_s( &tm_now , &now );

        // 현재 시간에서 월을 하나 더함
        if ( tm_now.tm_mon == 11 ) {  // 12월인 경우
            tm_now.tm_mon = 0;  // 1월로 설정
            tm_now.tm_year += 1;  // 년도 하나 더함
        }
        else {
            tm_now.tm_mon += 1;  // 다음 달로 설정
        }

        // 다음 달 1일 00시의 시간을 구하기
        tm_now.tm_mday = 1;
        tm_now.tm_hour = 0;
        tm_now.tm_min = 0;
        tm_now.tm_sec = 0;

        std::time_t next_month_start = std::mktime( &tm_now );
        return next_month_start;
    }

    static std::time_t TomorrowStartTimeTM()
    {
        // 현재 시간 구하기
        std::time_t now = std::time( nullptr );
        std::tm tm_now;
        localtime_s( &tm_now , &now );

        // 오늘 00시의 시간을 구하기
        tm_now.tm_hour = 0;
        tm_now.tm_min = 0;
        tm_now.tm_sec = 0;
        std::time_t today_midnight = std::mktime( &tm_now );

        // 내일 00시의 시간을 구하기
        tm_now.tm_mday += 1;  // 하루를 더함
        std::time_t tomorrow_midnight = std::mktime( &tm_now );
        return tomorrow_midnight;
    }

    static std::string TomorrowStartTimeString()
    {
        std::time_t tomorrowStartTime = TomorrowStartTimeTM();
        return TMToString( tomorrowStartTime );
    }

    static std::time_t SetTimeToFiveAM()
    {
        // 현재 시간 구하기
        std::time_t now = std::time( nullptr );
        std::tm tm_now;
        localtime_s( &tm_now , &now );

        // 오늘 05시의 시간을 구하기
        tm_now.tm_hour = 5;
        tm_now.tm_min = 0;
        tm_now.tm_sec = 0;
        std::time_t today_five_am = std::mktime( &tm_now );

        return today_five_am;
    }

    static std::time_t FiveAMTimeTM()
    {
        return SetTimeToFiveAM();
    }

    static std::string TMToString( const std::time_t& _time )
    {
        // Convert to KST timezone
        std::tm tm;
        localtime_s( &tm , &_time );

        // Format the date and time string
        std::ostringstream oss;
        oss << std::put_time( &tm , "%Y-%m-%d %H:%M:%S" );
        return oss.str();
    }

    static std::time_t AddDays( std::time_t input_time , const int& addDays )
    {
        // std::time_t를 std::tm으로 변환
        struct tm tmNow;
        localtime_s( &tmNow , &input_time );

        // 원하는 만큼 시간을 더하기
        tmNow.tm_hour += ( addDays * 24 );

        // 변경된 tm 구조체를 다시 std::time_t로 변환
        return std::mktime( &tmNow );
    }

    static std::string OneYearLaterTime()
    {
        // 현재 시간 구하기
        std::time_t now = std::time( nullptr );
        std::tm tm_now;
        localtime_s( &tm_now , &now );

        // 1년 후의 시간 계산
        tm_now.tm_year += 1; // 1년 추가
        std::time_t one_year_later = std::mktime( &tm_now );

        // std::time_t를 std::string으로 변환
        std::stringstream ss;
        ss << std::put_time( &tm_now , "%Y-%m-%d %H:%M:%S" );
        return ss.str();
    }

    static uint64 GetCurrentTimeInMilliseconds()
    {
        // 현재 시간을 얻고, epoch(1970년 1월 1일 00:00:00 UTC)부터의 밀리초 단위로 변환
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast< std::chrono::milliseconds >( duration ).count();
    }
      
    //받아온 시간 파싱
    static bool parseTime( const std::string& time_str , struct tm& time_info ) {
        std::istringstream ss( time_str );
        ss >> std::get_time( &time_info , "%Y-%m-%d %H:%M:%S" );
        return !ss.fail();
    }

    //현재시간에 해당하는지 체크
    static bool isCurrentTimeWithin( const std::string& start_time , const std::string& end_time ) {
        time_t now = time( nullptr );
        struct tm start_tm = {};
        struct tm end_tm = {};

        if ( !parseTime( start_time , start_tm ) || !parseTime( end_time , end_tm ) ) {
            std::cerr << "Failed to parse time." << std::endl;
            return false;
        }

        time_t start = mktime( &start_tm );
        time_t end = mktime( &end_tm );

        return now >= start && now <= end;
    }

    static std::string TodayDate()
    {
        auto to_day = TodayStartTimeTM();

        // 다음 달 1일 0시의 시간을 문자열로 변환
        std::tm tm_today;
        localtime_s( &tm_today , &to_day );

        std::stringstream ss;
        ss << std::put_time( &tm_today , "%Y-%m-%d" );
        return ss.str();
    }

    static std::string TodayDate(std::string time_string )
    {
        // 문자열을 시간으로 변환
        std::tm tm = {};
        std::istringstream ss( time_string );
        ss >> std::get_time( &tm , "%Y-%m-%d %H:%M:%S" );
        std::time_t t = std::mktime( &tm );

        // 다음 달 1일 0시의 시간을 문자열로 변환
        std::tm tm_today;
        localtime_s( &tm_today , &t );

        std::stringstream ss2;
        ss2 << std::put_time( &tm_today , "%Y-%m-%d" );
        return ss2.str();
    }

    static std::string NextdayDate( std::string time_string )
    {
        // 문자열을 시간으로 변환
        std::tm tm = {};
        std::istringstream ss( time_string );
        ss >> std::get_time( &tm , "%Y-%m-%d %H:%M:%S" );
        std::time_t t = std::mktime( &tm );

        std::tm tm_today;
        localtime_s( &tm_today , &t );

        tm_today.tm_mday += 1;  // 하루를 더함
        std::time_t tomorrow_midnight = std::mktime( &tm_today );

        // Convert to KST timezone
        std::tm mid_tm;
        localtime_s( &mid_tm , &tomorrow_midnight );

        std::stringstream ss2;
        ss2 << std::put_time( &mid_tm , "%Y-%m-%d" );
        return ss2.str();
    }

    static std::string ThisMondayDate()
    {
        // 현재 시간 구하기
        std::time_t now = std::time( nullptr );
        std::tm tm_now;
        localtime_s( &tm_now , &now );

        // 0 은 일요일 
        int days_to_monday = ( tm_now.tm_wday == 0 ) ? 6 : tm_now.tm_wday - 1;
        tm_now.tm_mday -= days_to_monday;

        // 변경된 날짜를 반영
        std::time_t monday = std::mktime( &tm_now );

        // 다음 달 1일 0시의 시간을 문자열로 변환
        std::tm tm_monday;
        localtime_s( &tm_monday , &monday );

        std::stringstream ss;
        ss << std::put_time( &tm_monday , "%Y-%m-%d" );
        return ss.str();
    }

    static std::string LastMondayDate()
    {
        // 현재 시간 구하기
        std::time_t now = std::time( nullptr );
        std::tm tm_now;
        localtime_s( &tm_now , &now );

        // 0 은 일요일 
        int days_to_monday = ( tm_now.tm_wday == 0 ) ? 6 : tm_now.tm_wday - 1;
        tm_now.tm_mday -= days_to_monday + 7;

        // 변경된 날짜를 반영
        std::time_t last_monday = std::mktime( &tm_now );

        // 다음 달 1일 0시의 시간을 문자열로 변환
        std::tm tm_last_monday;
        localtime_s( &tm_last_monday , &last_monday );

        std::stringstream ss;
        ss << std::put_time( &tm_last_monday , "%Y-%m-%d" );
        return ss.str();
    }

    static std::string ThisMondayDate(const std::string& date)
    {
        std::tm tm_date = {};  // 날짜를 저장할 구조체

        // 문자열을 std::tm으로 파싱 (예: "2024-10-22")
        std::istringstream ss( date );
        ss >> std::get_time( &tm_date , "%Y-%m-%d" );

        if ( ss.fail() ) {
            std::cout << "Failed to parse date string." << std::endl;
        }

        // 0 은 일요일 
        int days_to_monday = ( tm_date.tm_wday == 0 ) ? 6 : tm_date.tm_wday - 1;
        tm_date.tm_mday -= days_to_monday;

        // 변경된 날짜를 반영
        std::time_t monday = std::mktime( &tm_date );

        // 다음 달 1일 0시의 시간을 문자열로 변환
        std::tm tm_monday;
        localtime_s( &tm_monday , &monday );

        std::stringstream _ss;
        _ss << std::put_time( &tm_monday , "%Y-%m-%d" );
        return _ss.str();
    }

    static std::string ThisSundayDate()
    {
        // 현재 시간 구하기
        std::time_t now = std::time( nullptr );
        std::tm tm_now;
        localtime_s( &tm_now , &now );

        // 0 은 일요일 
        int days_to_monday = ( tm_now.tm_wday == 0 ) ? 6 : tm_now.tm_wday - 1;
        tm_now.tm_mday -= days_to_monday;

        // 변경된 날짜를 반영
        std::time_t monday = std::mktime( &tm_now );

        // 다음 달 1일 0시의 시간을 문자열로 변환
        std::tm tm_monday;
        localtime_s( &tm_monday , &monday );

        tm_monday.tm_mday += 6;

        std::time_t sunday_time = std::mktime( &tm_monday );
        std::tm tm_sunday;
        localtime_s( &tm_sunday , &sunday_time );

        std::stringstream ss;
        ss << std::put_time( &tm_sunday , "%Y-%m-%d" );
        return ss.str();
    }

    static std::string ThisSundayDate( const std::string& date )
    {
        std::tm tm_date = {};  // 날짜를 저장할 구조체

        // 문자열을 std::tm으로 파싱 (예: "2024-10-22")
        std::istringstream ss( date );
        ss >> std::get_time( &tm_date , "%Y-%m-%d" );

        if ( ss.fail() ) {
            std::cout << "Failed to parse date string." << std::endl;
        }

        // 0 은 일요일 
        int days_to_monday = ( tm_date.tm_wday == 0 ) ? 6 : tm_date.tm_wday - 1;
        tm_date.tm_mday -= days_to_monday;

        // 변경된 날짜를 반영
        std::time_t monday = std::mktime( &tm_date );

        // 다음 달 1일 0시의 시간을 문자열로 변환
        std::tm tm_monday;
        localtime_s( &tm_monday , &monday );

        tm_monday.tm_mday += 6;

        std::time_t sunday_time = std::mktime( &tm_monday );
        std::tm tm_sunday;
        localtime_s( &tm_sunday , &sunday_time );

        std::stringstream _ss;
        _ss << std::put_time( &tm_sunday , "%Y-%m-%d" );
        return _ss.str();
    }

};
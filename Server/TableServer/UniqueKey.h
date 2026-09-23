#pragma once

#include "TableServerHeader.h"

#include <iostream>
#include <ctime>
#include <chrono>

class UniqueKey
{
public:

    static const int Bit7Value = 128;

    //static std::tuple<uint32 , int> GetTotalSecAndMiliSec()
    //{
    //    // 현재 시간을 초와 밀리초로 변환하여 반환하는 예시 코드
    //    std::time_t now = std::time( nullptr );
    //    std::tm local_tm;
    //    if ( localtime_s( &local_tm , &now ) != 0 ) {
    //        // localtime_s 함수 호출 실패 처리
    //        throw std::overflow_error( "UniqueKey.GetTotalSecAndMiliSec localtime_s failed" );
    //    }
    //    uint32 total_sec = local_tm.tm_sec;
    //    int total_msec = 0; // 이 부분은 실제로 밀리초를 구하는 코드로 대체되어야 합니다.
    //    return std::make_tuple( total_sec , total_msec );
    //}

    // 기준시간 2024년 1월 1일 0시
    static std::tuple<uint32 , int> GetTotalSecAndMiliSec()
    {
        std::tm start_time = {};
        start_time.tm_year = 2024 - 1900; // years since 1900
        start_time.tm_mon = 0; // January
        start_time.tm_mday = 1; // 1st day
        start_time.tm_hour = 0; // 0 hour

        // 2024년 1월 1일 0시를 시간으로 변환
        auto start_time_point = std::chrono::system_clock::from_time_t( std::mktime( &start_time ) );

        // 현재 시간을 밀리초로 변환하여 반환하는 코드
        auto now = std::chrono::system_clock::now();
        auto duration = now - start_time_point;

        // 초 단위와 밀리초 단위로 변환
        auto seconds = std::chrono::duration_cast< std::chrono::seconds >( duration );
        auto milliseconds = std::chrono::duration_cast< std::chrono::milliseconds >( duration - seconds );

        // 초와 밀리초를 가져옴
        uint32_t total_sec = seconds.count();
        int total_msec = static_cast< int >( milliseconds.count() );

        return std::make_tuple( total_sec , total_msec );
    }

    static uint32 GetCurMSecToday()
    {
        // 현재 시간을 가져옵니다.
        auto now = std::chrono::system_clock::now();

        // 현재 시간을 밀리초로 변환합니다.
        auto milliseconds_since_epoch = std::chrono::duration_cast< std::chrono::milliseconds >( now.time_since_epoch() ).count();

        // 하루의 시작 시간을 가져옵니다.
        auto start_of_day = std::chrono::system_clock::from_time_t( std::chrono::system_clock::to_time_t( now ) );
        auto start_of_day_milliseconds = std::chrono::duration_cast< std::chrono::milliseconds >( start_of_day.time_since_epoch() ).count();

        // 현재 시간과 하루의 시작 시간 간의 차이를 계산하여 현재 시간이 하루 중 몇 밀리초인지 구합니다.
        auto milliseconds_since_start_of_day = milliseconds_since_epoch - start_of_day_milliseconds;
        return milliseconds_since_start_of_day;
    }

    // player_idx 가 unsigned 30 bit 곧 1,073,741,823 까지 표현
    // Spin 시도시에 생성되는 Unique 키 생성기
    // spin_sequence 없이 호출하면 SpinRes 저장을 위한 고유키
    // spin_sequence 를 1부터 호출하면 
    static uint64 SpinUniqueKey( uint32 player_idx )
    {
        if ( player_idx > UINT32_MAX )
            throw std::overflow_error( "cUniqueKey.SpinUniqueKey overflow account_idx" );

        uint64 _unique_key = 0;
        uint64 _temp_acccount_key = static_cast< uint64 >( player_idx ) << 32; // 32 에서 2비트 더 댕긴다.
        uint32 _cur_msec = GetCurMSecToday();
        _unique_key = _temp_acccount_key + _cur_msec;

        return static_cast< uint64 >( _unique_key );
    }
};
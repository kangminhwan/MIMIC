#pragma once
#include <iostream>
#include <array>
#include <chrono>
#include <string>
#include <ctime>
#include <optional>
#include "../Include/Netlib/Common/cSingleton.h"


class IpTimeQueue {
private:
    static constexpr int MAX_SIZE = 10; // 최대 저장 개수
    std::array<std::pair<std::string , std::chrono::system_clock::time_point> , MAX_SIZE> data;
    int front;
    int rear;
    int count;
    int MAX_TIME = 2;

public:
    IpTimeQueue() : front( 0 ) , rear( 0 ) , count( 0 ) {}

    int GetMaxTime() { return MAX_TIME; }

    void push( const std::string& ip ) {
        auto now = std::chrono::system_clock::now();

        data[ rear ] = { ip, now };
        rear = ( rear + 1 ) % MAX_SIZE;

        if ( count < MAX_SIZE ) {
            count++;
        }
        else {
            front = ( front + 1 ) % MAX_SIZE; // 가장 오래된 데이터 덮어쓰기
        }
    }

    int findIP( const std::string& ip ) const {
        int index = front;
        auto now = std::chrono::system_clock::now(); // 현재 시간

        for ( int i = 0; i < count; ++i ) {
            if ( data[ index ].first == ip ) {
                auto duration = std::chrono::duration_cast< std::chrono::seconds >( now - data[ index ].second );
                return static_cast< int >( duration.count() ); // 경과 시간(초) 반환
            }
            index = ( index + 1 ) % MAX_SIZE;
        }
        return 10; // 해당 IP가 없으면 0 반환
    }

};
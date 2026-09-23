#pragma once
#include <iostream>
#include <vector>
#include <array>
using namespace std;

class CircularVector {
private:
    vector<int> buffer;         // 최신 500개 숫자를 저장할 버퍼
    const int capacity;         // 버퍼 최대 크기 (여기서는 500)
    int head;                   // 현재 버퍼에서 가장 오래된 원소의 인덱스
    int size;                   // 현재 버퍼에 저장된 원소의 개수
    array<int , 38> freq;        // 0~37번 숫자의 빈도(출현 횟수) 저장

public:
    // 생성자: capacity를 500으로 초기화
    CircularVector( int cap = 500 )
        : capacity( cap ) , head( 0 ) , size( 0 )
    {
        buffer.resize( capacity );
        freq.fill( 0 );
    }

    // insert 함수: 새 숫자 삽입
    void insert( int number ) {
        // 유효한 숫자인지 검사 (여기서는 0~37)
        if ( number < 0 || number >= static_cast< int >( freq.size() ) ) {
            cerr << "Invalid number: " << number << endl;
            return;
        }

        if ( size < capacity ) {
            // 버퍼가 아직 가득 차지 않았다면,
            // (head + size) % capacity 위치에 새 숫자를 삽입
            int index = ( head + size ) % capacity;
            buffer[ index ] = number;
            size++;
            freq[ number ]++;
        }
        else {
            // 버퍼가 가득 찬 경우:
            // 가장 오래된 숫자(buffer[head])의 빈도 감소 후,
            // 해당 위치에 새 숫자를 덮어쓰고, 새 숫자의 빈도 증가
            int oldest = buffer[ head ];
            freq[ oldest ]--;
            buffer[ head ] = number;
            freq[ number ]++;
            // head를 한 칸 뒤로 이동 (원형 버퍼 형태)
            head = ( head + 1 ) % capacity;
        }
    }

    // 현재 버퍼 내의 모든 숫자 출력 (디버깅용)
    void printBuffer() const {
        cout << "Current buffer contents: ";
        for ( int i = 0; i < size; i++ ) {
            int index = ( head + i ) % capacity;
            cout << buffer[ index ] << " ";
        }
        cout << endl;
    }

    // 각 숫자의 빈도를 출력
    void printFrequencies() const {
        cout << "Frequencies in current window:" << endl;
        for ( int i = 0; i < static_cast< int >( freq.size() ); i++ ) {
            cout << "Number " << i << ": " << freq[ i ] << endl;
        }
    }

    std::vector<int> GetLastNums( int count )
    {
        std::vector<int> result;

        // 예: 현재 저장된 요소 개수를 size라고 가정한다면,
        // count가 size보다 클 경우 size로 제한할 수 있습니다.
        int actualCount = std::min( count , size ); // size는 클래스 멤버 변수로 현재 저장된 요소 수

        for ( size_t i = 0; i < actualCount; i++ )
        {
            int index = ( head - 1 - i + capacity ) % capacity;
            result.push_back( buffer[ index ] );  // buffer는 링 버퍼를 나타내는 클래스 멤버 변수
        }

        return result;
    }

};
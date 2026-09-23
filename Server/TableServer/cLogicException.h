#pragma once
#include <iostream>
#include <stdexcept>

class cLogicException : public std::exception {
public:
    cLogicException( const char* message ) : errorMessage( message ) {}

    // 상속된 가상 함수 what을 오버라이드하여 예외 메시지를 반환합니다.
    const char* what() const noexcept override {
        return errorMessage.c_str();
    }

private:
    std::string errorMessage;
};
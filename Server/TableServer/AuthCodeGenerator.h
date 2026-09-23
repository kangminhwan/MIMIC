#pragma once
#include <iostream>
#include <random>
#include <string>

class AuthCodeGenerator 
{
public:
    AuthCodeGenerator()
    {
        // 초기화할 문자열이 더 있다면 이곳에 추가 하십시요.
        chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    }

    std::string generate(int length)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> distribution(0, chars.size() - 1);

        std::string authCode;
        for (int i = 0; i < length; ++i) {
            authCode += chars[distribution(gen)];
        }

        return authCode;
    }

private:
    std::string chars;
};
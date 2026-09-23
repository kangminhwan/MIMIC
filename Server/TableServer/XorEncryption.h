#include "LocalSettings.h"
#pragma once
#include <string>

class cXorEncryption
{
public:
	static std::string EncryptDecrypt( const std::string& input ) {

        static std::string key = MIMIC_XOR_KEY;
        if (key.empty()) throw std::runtime_error("Configure XOR key in LocalSettings.local.h");

        std::string output = input;
        for ( size_t i = 0; i < input.size(); ++i ) {
            output[ i ] = input[ i ] ^ key[ i % key.size() ];
        }
        return output;
    }
};

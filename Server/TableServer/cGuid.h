#pragma once
#include <windows.h>
#include <objbase.h>
#include <iostream>

class cGuild
{
public:
    static std::string GetGuid(const int guild_length = 40)
    {
        GUID newGuid;
        if ( CoCreateGuid( &newGuid ) == S_OK ) {
            // GUID 생성 성공
            wchar_t guidStr[ 40 ]; // Wide character buffer to store the GUID string
            if ( StringFromGUID2( newGuid , guidStr , guild_length ) > 0 ) {
                // Convert the wide character GUID to a narrow character string
                int bufferSize = WideCharToMultiByte( CP_ACP , 0 , guidStr , -1 , nullptr , 0 , nullptr , nullptr );
                if ( bufferSize > 0 ) {
                    std::string narrowGuidStr( bufferSize , 0 );
                    WideCharToMultiByte( CP_ACP , 0 , guidStr , -1 , &narrowGuidStr[ 0 ] , bufferSize , nullptr , nullptr );
                    return narrowGuidStr;
                }
            }
        }

        // Return an empty string if the GUID creation failed
        return std::string();
    }
};
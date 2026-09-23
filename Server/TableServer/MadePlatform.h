#include "LocalSettings.h"
#pragma once
#include "TableServerHeader.h"

#include <iostream>
#include <string>
#include <regex>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include <algorithm>
#include <random> // std::random_device, std::mt19937
#include <iostream>
#include <iomanip>
#include <sstream>

class MADEPlatform
{
private:
    // Local encryption settings are defined separately.
    // Local encryption settings are defined separately.

public:

    // ID À¯È¿¼º °Ë»ç
    static bool ValidateId( const std::string& id )
    {
        // Á¤±Ô Ç¥Çö½ÄÀ» »ç¿ëÇÏ¿© ¿µ¹®ÀÚ¿Í ¼ıÀÚ·Î ÀÌ·ç¾îÁø 5ÀÚ¿¡¼­ 12ÀÚÀÇ ¹®ÀÚ¿­ÀÎÁö È®ÀÎ
        std::regex pattern( "^[a-zA-Z0-9]{5,12}$" );
        return std::regex_match( id , pattern );
    }
    
    // PASSWORD À¯È¿¼º °Ë»ç
    static bool isValidPassword( const std::string& password )
    {
        // ºñ¹Ğ¹øÈ£°¡ 8~16ÀÚÀÇ ±æÀÌÀÎÁö È®ÀÎ
        if ( password.length() < 8 || password.length() > 16 ) {
            return false;
        }

        // ºñ¹Ğ¹øÈ£¿¡ ¿µ¹® ´ë¹®ÀÚ ¶Ç´Â ¿µ¹® ¼Ò¹®ÀÚ°¡ Æ÷ÇÔµÇ¾î ÀÖ´ÂÁö È®ÀÎ
        if ( ( !std::regex_search( password , std::regex( "[A-Z]" ) ) ) && ( !std::regex_search( password , std::regex( "[a-z]" ) ) ) ) {
            return false;
        }

        // ºñ¹Ğ¹øÈ£¿¡ ¿µ¹® ¼Ò¹®ÀÚ°¡ Æ÷ÇÔµÇ¾î ÀÖ´ÂÁö È®ÀÎ
        /*if ( !std::regex_search( password , std::regex( "[a-z]" ) ) ) {
            return false;
        }*/

        // ºñ¹Ğ¹øÈ£¿¡ ¼ıÀÚ°¡ Æ÷ÇÔµÇ¾î ÀÖ´ÂÁö È®ÀÎ
        if ( !std::regex_search( password , std::regex( "[0-9]" ) ) ) {
            return false;
        }

        // ºñ¹Ğ¹øÈ£¿¡ Æ¯¼ö¹®ÀÚ°¡ Æ÷ÇÔµÇ¾î ÀÖ´ÂÁö È®ÀÎ
        if ( !std::regex_search( password , std::regex( "[!@#$%^&*()-_+=]" ) ) ) {
            return false;
        }

        // ¸ğµç Á¶°ÇÀ» ¸¸Á·ÇÏ¸é À¯È¿ÇÑ ºñ¹Ğ¹øÈ£·Î ÆÇ´Ü
        return true;
        // ºñ¹Ğ¹øÈ£ ±ÔÄ¢À» ³ªÅ¸³»´Â Á¤±Ô Ç¥Çö½Ä
        //std::regex pattern( "^(?=.*[a-z])(?=.*[A-Z])(?=.*\\d)(?=.*[!@#$%^&*()-_+=])[A-Za-z\\d!@#$%^&*()-_+=]{8,16}$" );
        //return std::regex_match( password , pattern );
    }

    static bool isValidNickName( const std::string& str )
    {
        // Á¤±Ô Ç¥Çö½Ä ÆĞÅÏ: ÇÑ±Û ¶Ç´Â ¿µ¹® ´ë¹®ÀÚ, ÇÑ±Û ¶Ç´Â ¿µ¹® ¼Ò¹®ÀÚ, ¼ıÀÚ Á¶ÇÕÀÇ 2¿¡¼­ 8ÀÚ »çÀÌ
        std::regex pattern( "[°¡-ÆRA-Za-z0-9]{2,8}" );
        return std::regex_match( str , pattern );
    }

    // Æ¯¼ö¹®ÀÚ°¡ Æ÷ÇÔµÇ¾î ÀÖ´ÂÁö È®ÀÎ
    static bool ContainsSpecialCharacters( const std::string& str ) {
        if ( std::regex_search( str , std::regex( "[!@#$%^&*()-_+=]" ) ) ) {
            return false;
        }
    }

    // Password ¾ÏÈ£È­ ÇÔ¼ö
    static std::string EncryptPassword( const std::string& password )
    {
        // AES 128 ºñ¹ĞÅ°¿Í ÃÊ±âÈ­ º¤ÅÍ ¼³Á¤
        static const std::string key = MIMIC_PASSWORD_KEY;
        static const std::string iv = MIMIC_PASSWORD_IV;
        if (key.size() != 16 || iv.size() != 16) throw std::runtime_error("Configure password key and IV in LocalSettings.local.h");

        // OpenSSL ¶óÀÌºê·¯¸® ÃÊ±âÈ­
        OpenSSL_add_all_algorithms();

        // ¾ÏÈ£È­ ÄÁÅØ½ºÆ® »ı¼º
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if ( !ctx ) {
            return ""; // ¾ÏÈ£È­ ÄÁÅØ½ºÆ® »ı¼º ½ÇÆĞ
        }

        // ¾ÏÈ£È­ ÃÊ±âÈ­
        if ( EVP_EncryptInit_ex( ctx , EVP_aes_128_cbc() , nullptr , ( const unsigned char* ) key.c_str() , ( const unsigned char* ) iv.c_str() ) != 1 ) {
            EVP_CIPHER_CTX_free( ctx );
            return ""; // ¾ÏÈ£È­ ÃÊ±âÈ­ ½ÇÆĞ
        }

        // ¾ÏÈ£È­ÇÒ µ¥ÀÌÅÍ ±æÀÌ °è»ê
        int encryptedLength = password.length() + EVP_CIPHER_CTX_block_size( ctx );

        // ¾ÏÈ£È­ °á°ú¸¦ ÀúÀåÇÒ ¹öÆÛ »ı¼º
        std::string encrypted( encryptedLength , '\0' );

        // ¾ÏÈ£È­ ½ÇÇà
        int actualEncryptedLength;
        if ( EVP_EncryptUpdate( ctx , ( unsigned char* ) &encrypted[ 0 ] , &actualEncryptedLength , ( const unsigned char* ) password.c_str() , password.length() ) != 1 ) {
            EVP_CIPHER_CTX_free( ctx );
            return ""; // ¾ÏÈ£È­ ½ÇÆĞ
        }

        // ¾ÏÈ£È­ Á¾·á
        int finalEncryptedLength;
        if ( EVP_EncryptFinal_ex( ctx , ( unsigned char* ) &encrypted[ actualEncryptedLength ] , &finalEncryptedLength ) != 1 ) {
            EVP_CIPHER_CTX_free( ctx );
            return ""; // ¾ÏÈ£È­ Á¾·á ½ÇÆĞ
        }

        // ¾ÏÈ£È­ °á°úÀÇ ½ÇÁ¦ ±æÀÌ °»½Å
        encrypted.resize( actualEncryptedLength + finalEncryptedLength );

        // ¾ÏÈ£È­ ÄÁÅØ½ºÆ® ÇØÁ¦
        EVP_CIPHER_CTX_free( ctx );

        return encrypted;
    }


    // ¾ÈÀüÇÑ ¹«ÀÛÀ§ ¹ÙÀÌÆ® ¹è¿­À» »ı¼ºÇÏ´Â ÇÔ¼ö
    static std::string GenerateRandomBytes( size_t length = 16 )
    {
        static std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::random_device rd;
        std::mt19937 gen( rd() );
        std::uniform_int_distribution<int> distribution( 0 , chars.size() - 1 );

        std::string random_string;
        for ( int i = 0; i < length; ++i ) {
            random_string += chars[ distribution( gen ) ];
        }

        return random_string;
    }

    static std::string GenerateSha256( const std::string& input )
    {
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        unsigned char digest[ SHA256_DIGEST_LENGTH ];
        unsigned int digest_len;

        EVP_DigestInit_ex( mdctx , EVP_sha256() , nullptr );
        EVP_DigestUpdate( mdctx , input.c_str() , input.length() );
        EVP_DigestFinal_ex( mdctx , digest , &digest_len );

        EVP_MD_CTX_free( mdctx );

        std::stringstream ss;
        for ( unsigned int i = 0; i < digest_len; i++ ) {
            ss << std::hex << std::setw( 2 ) << std::setfill( '0' ) << ( int ) digest[ i ];
        }

        return ss.str();
    }
};

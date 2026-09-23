#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include "../Include/Netlib/Common/cSingleton.h"
class TrieNode {
public:
    bool isEndOfWord;
    std::unordered_map<wchar_t , std::unique_ptr<TrieNode>> children;

    TrieNode() : isEndOfWord( false ) {}
};

class Trie {
public:
    // 금칙어 삽입 메서드 (유니코드 지원)
    void insert( const std::wstring& word ) {
        TrieNode* node = root.get();
        for ( wchar_t ch : word ) {
            if ( node->children.find( ch ) == node->children.end() ) {
                node->children[ ch ] = std::make_unique<TrieNode>();
            }
            node = node->children[ ch ].get();
        }
        node->isEndOfWord = true;
    }

    // 주어진 아이디에 금칙어가 포함되어 있는지 체크
    bool containsRestrictedWord( const std::wstring& username ) 
    {
        for ( int i = 0; i < username.size(); ++i ) 
        {
            TrieNode* node = root.get();
            for ( int j = i; j < username.size(); ++j ) 
            {
                wchar_t ch = username[ j ];
                if ( node->children.find( ch ) == node->children.end() ) break;
                node = node->children[ ch ].get();
                if ( node->isEndOfWord ) return true;
            }
        }
        return false;
    }

private:
    std::unique_ptr<TrieNode> root = std::make_unique<TrieNode>();
};
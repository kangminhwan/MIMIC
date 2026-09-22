#include "Netlib/Transport.hpp"
#include <windows.h>
#include <winhttp.h>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>
namespace mimic::net {
namespace {
bool exact(SOCKET s,char* data,int size,bool writing) {
    while(size>0) {
        int n=writing ? ::send(s,data,size,0) : recv(s,data,size,0);
        if(n<=0) return false;
        data+=n;size-=n;
    }
    return true;
}
struct HttpHandle { HINTERNET h; ~HttpHandle(){ if(h) WinHttpCloseHandle(h); } operator HINTERNET() const{return h;} };

// Host is always little-endian Windows x86/x64, so header fields are just raw byte copies —
// no htonl/byte-swapping, unlike the big-endian 4-byte length prefix this format replaces.
void writeField(unsigned char* buffer,size_t offset,uint32_t value) { std::memcpy(buffer+offset,&value,sizeof(value)); }
uint32_t readField(const unsigned char* buffer,size_t offset) { uint32_t value; std::memcpy(&value,buffer+offset,sizeof(value)); return value; }

void applyXor(std::string& body) {
    for(char& byte:body) byte=static_cast<char>(static_cast<unsigned char>(byte)^PayloadXorMask);
}
}
bool Connection::send(const protocol::Envelope& message) {
    std::string body;
    if(!message.SerializeToString(&body) || body.empty() || body.size()>MaxFrame) return false;
    unsigned char header[HeaderSize];
    writeField(header,0,HeaderMagic);
    writeField(header,4,0); // entity: unused by MIMIC, always 0
    // Zero nonce marks a server-initiated push; responses echo the request's id (validated to
    // fit uint32 when the request was received).
    writeField(header,8,static_cast<uint32_t>(message.request_id()));
    writeField(header,12,static_cast<uint32_t>(message.payload_case()));
    writeField(header,16,static_cast<uint32_t>(body.size()));
    applyXor(body); // encode a copy; caller's Envelope is untouched
    std::lock_guard lock(writeMutex);
    // Allocated under writeMutex, immediately before the actual socket writes below, so the
    // sequence numbers observed on the wire are strictly ordered the same way concurrent send()
    // calls on this connection are ordered — allocating it earlier (outside the lock) could let
    // two threads interleave such that a higher sequence number reaches the wire first.
    writeField(header,20,++sendSequence);
    return exact(socket,reinterpret_cast<char*>(header),static_cast<int>(HeaderSize),true)
        && exact(socket,body.data(),static_cast<int>(body.size()),true);
}
bool receive(SOCKET socket,protocol::Envelope& message) {
    unsigned char header[HeaderSize];
    if(!exact(socket,reinterpret_cast<char*>(header),static_cast<int>(HeaderSize),false)) return false;
    if(readField(header,0)!=HeaderMagic) return false;
    if(readField(header,4)!=0) return false; // entity is reserved at 0 by this contract
    uint32_t payloadSize=readField(header,16);
    if(payloadSize==0 || payloadSize>MaxFrame) return false;
    std::vector<char> body(payloadSize);
    if(!exact(socket,body.data(),static_cast<int>(payloadSize),false)) return false;
    std::string decoded(body.begin(),body.end());
    applyXor(decoded);
    if(!message.ParseFromString(decoded)) return false;
    if(message.request_id()>std::numeric_limits<uint32_t>::max()) return false;
    if(readField(header,8)!=static_cast<uint32_t>(message.request_id())) return false;
    if(readField(header,12)!=static_cast<uint32_t>(message.payload_case())) return false;
    return true;
}
protocol::SessionReply validate(const std::string& token,unsigned short port) {
    if(token.size()!=64) throw std::runtime_error("Invalid session token");
    HttpHandle session{WinHttpOpen(L"MIMIC/0.1",WINHTTP_ACCESS_TYPE_NO_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
    if(!session.h) throw std::runtime_error("Platform connection unavailable");
    WinHttpSetTimeouts(session,1500,1500,1500,1500);
    HttpHandle connection{WinHttpConnect(session,L"127.0.0.1",port,0)};
    HttpHandle request{WinHttpOpenRequest(connection,L"POST",L"/sessions/validate",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,0)};
    protocol::ValidateSessionRequest input; input.set_session_token(token);auto body=input.SerializeAsString();
    if(!request.h || !WinHttpSendRequest(request,L"Content-Type: application/x-protobuf\r\n",static_cast<DWORD>(-1),body.data(),static_cast<DWORD>(body.size()),static_cast<DWORD>(body.size()),0) || !WinHttpReceiveResponse(request,nullptr))
        throw std::runtime_error("Platform connection unavailable");
    DWORD status=0,len=sizeof(status);
    if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&len,WINHTTP_NO_HEADER_INDEX) || status!=200)
        throw std::runtime_error("Session expired or invalid");
    std::string output;char buffer[1024];DWORD read=0;
    for(;;) {
        if(!WinHttpReadData(request,buffer,sizeof(buffer),&read)) throw std::runtime_error("Platform read failed");
        if(!read) break;
        output.append(buffer,read);if(output.size()>4096) throw std::runtime_error("Platform response too large");
    }
    protocol::SessionReply result;
    if(!result.ParseFromString(output) || result.player_id().empty()) throw std::runtime_error("Invalid platform response");
    return result;
}
}

#include "Netlib/Transport.hpp"
#include <windows.h>
#include <winhttp.h>
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
}
bool Connection::send(const protocol::Envelope& message) {
    std::string body;
    if(!message.SerializeToString(&body) || body.empty() || body.size()>MaxFrame) return false;
    uint32_t length=htonl(static_cast<uint32_t>(body.size()));
    std::lock_guard lock(writeMutex);
    return exact(socket,reinterpret_cast<char*>(&length),4,true) && exact(socket,body.data(),static_cast<int>(body.size()),true);
}
bool receive(SOCKET socket,protocol::Envelope& message) {
    uint32_t size=0;
    if(!exact(socket,reinterpret_cast<char*>(&size),4,false)) return false;
    size=ntohl(size);
    if(size==0 || size>MaxFrame) return false;
    std::vector<char> body(size);
    return exact(socket,body.data(),static_cast<int>(size),false) && message.ParseFromArray(body.data(),static_cast<int>(size));
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

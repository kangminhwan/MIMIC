#include "Netlib/Transport.hpp"
#include "Holdem/Table.hpp"
#include <algorithm>
#include <atomic>
#include <future>
#include <iostream>
#include <memory>
#include <vector>

namespace {
std::atomic<SOCKET> listener{INVALID_SOCKET};
BOOL WINAPI onConsole(DWORD type) {
    if(type==CTRL_C_EVENT || type==CTRL_BREAK_EVENT || type==CTRL_CLOSE_EVENT) {
        SOCKET value=listener.exchange(INVALID_SOCKET);if(value!=INVALID_SOCKET) closesocket(value);return TRUE;
    }
    return FALSE;
}
struct World {
    std::mutex mutex;
    mimic::Table table;
    std::vector<std::shared_ptr<mimic::net::Connection>> clients;
    void broadcast() {
        for(auto& client:clients) if(table.contains(client->playerId)) {
            mimic::protocol::Envelope event;event.set_protocol_version(1);
            *event.mutable_snapshot()=table.snapshot(client->playerId);
            if(!client->send(event)) client->stop();
        }
    }
};
void serve(const std::shared_ptr<mimic::net::Connection>& client,World& world,unsigned short platformPort) {
    uint64_t lastRequest=0;
    mimic::protocol::Envelope request;
    while(mimic::net::receive(client->socket,request)) {
        mimic::protocol::Envelope response;response.set_protocol_version(1);response.set_request_id(request.request_id());
        std::lock_guard lock(world.mutex);
        bool changed=false;
        try {
            if(request.protocol_version()!=1) throw std::runtime_error("Unsupported protocol version");
            if(request.request_id()==0 || request.request_id()<=lastRequest) throw std::runtime_error("Request IDs must increase");
            lastRequest=request.request_id();
            using E=mimic::protocol::Envelope;
            if(request.payload_case()==E::kAuthenticate) {
                if(!client->playerId.empty()) throw std::runtime_error("Already authenticated");
                auto session=mimic::net::validate(request.authenticate().session_token(),platformPort);
                for(auto& other:world.clients) if(other!=client && other->playerId==session.player_id()) throw std::runtime_error("Session already connected");
                if(world.table.contains(session.player_id())) throw std::runtime_error("Disconnected hand still settling; reconnect after the hand ends");
                client->playerId=session.player_id();client->displayName=session.display_name();
                response.mutable_authenticated()->set_player_id(client->playerId);
                response.mutable_authenticated()->set_display_name(client->displayName);
            } else {
                if(client->playerId.empty()) throw std::runtime_error("Authenticate first");
                switch(request.payload_case()) {
                case E::kListTables: {
                    auto info=response.mutable_lobby()->add_tables();info->set_table_id(1);info->set_name("MIMIC Holdem 10/20");
                    info->set_players(world.table.count());info->set_capacity(mimic::Table::Capacity);info->set_small_blind(10);info->set_big_blind(20);break;
                }
                case E::kJoinTable:
                    if(request.join_table().table_id()!=1) throw std::runtime_error("Unknown table");
                    world.table.join(client->playerId,client->displayName);response.mutable_acknowledged();changed=true;break;
                case E::kLeaveTable:
                    world.table.leave(client->playerId);response.mutable_acknowledged();changed=true;break;
                case E::kReady:
                    world.table.ready(client->playerId,request.ready().ready());response.mutable_acknowledged();changed=true;break;
                case E::kAction:
                    world.table.act(client->playerId,request.action());response.mutable_acknowledged();changed=true;break;
                case E::kPing: response.mutable_pong();break;
                default: throw std::runtime_error("Unsupported request");
                }
            }
        } catch(const std::exception& error) {
            response.mutable_error()->set_code("REQUEST_REJECTED");response.mutable_error()->set_message(error.what());
        }
        if(!client->send(response)) break;
        if(changed) world.broadcast();
    }
    client->stop();
    std::lock_guard lock(world.mutex);
    world.table.disconnect(client->playerId);
    std::erase(world.clients,client);world.broadcast();
}
unsigned short port(const char* value) {
    size_t used=0;int result=std::stoi(value,&used);
    if(value[used]!='\0' || result<1 || result>65535) throw std::runtime_error("Invalid port");
    return static_cast<unsigned short>(result);
}
}
int main(int argc,char** argv) {
    try {
        unsigned short tablePort=argc>1 ? port(argv[1]) : 7777;
        unsigned short platformPort=argc>2 ? port(argv[2]) : 5081;
        WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data)!=0) throw std::runtime_error("WSAStartup failed");
        SOCKET server=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(tablePort);address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        if(server==INVALID_SOCKET || bind(server,reinterpret_cast<sockaddr*>(&address),sizeof(address))==SOCKET_ERROR || listen(server,16)==SOCKET_ERROR)
            throw std::runtime_error("Cannot listen on loopback table port");
        listener=server;SetConsoleCtrlHandler(onConsole,TRUE);
        World world;std::vector<std::future<void>> workers;
        std::cout<<"MIMIC TableServer listening on 127.0.0.1:"<<tablePort<<std::endl;
        while(listener!=INVALID_SOCKET) {
            SOCKET socket=accept(server,nullptr,nullptr);if(socket==INVALID_SOCKET) break;
            std::erase_if(workers,[](auto& work){if(work.wait_for(std::chrono::seconds(0))==std::future_status::ready){work.get();return true;}return false;});
            if(workers.size()>=64){closesocket(socket);continue;}
            DWORD readTimeout=90000,writeTimeout=2000;BOOL noDelay=TRUE;
            setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&readTimeout),sizeof(readTimeout));
            setsockopt(socket,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<char*>(&writeTimeout),sizeof(writeTimeout));
            setsockopt(socket,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<char*>(&noDelay),sizeof(noDelay));
            auto client=std::make_shared<mimic::net::Connection>(socket);
            {std::lock_guard lock(world.mutex);world.clients.push_back(client);}
            workers.push_back(std::async(std::launch::async,[client,&world,platformPort]{serve(client,world,platformPort);}));
        }
        {std::lock_guard lock(world.mutex);for(auto& client:world.clients) client->stop();}
        for(auto& work:workers) work.get();
        WSACleanup();return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<std::endl;return 1;}
}

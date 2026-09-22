#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "mimic.pb.h"
#include <mutex>
#include <string>
namespace mimic::net {
constexpr uint32_t MaxFrame = 64 * 1024;
struct Connection {
    explicit Connection(SOCKET value) : socket(value) {}
    ~Connection() { closesocket(socket); }
    const SOCKET socket;
    std::string playerId, displayName;
    std::mutex writeMutex;
    bool send(const protocol::Envelope& message);
    void stop() const { shutdown(socket, SD_BOTH); }
};
bool receive(SOCKET socket, protocol::Envelope& message);
protocol::SessionReply validate(const std::string& token, unsigned short platformPort);
}

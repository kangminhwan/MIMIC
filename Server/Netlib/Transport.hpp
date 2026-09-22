#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "mimic.pb.h"
#include <cstdint>
#include <mutex>
#include <string>
namespace mimic::net {
constexpr uint32_t MaxFrame = 64 * 1024;

// Wire framing: a 24-byte little-endian header (6 uint32 fields) followed by an XOR-masked
// Envelope protobuf body. Layout and constants match the legacy cHeader/PacketHeader contract
// (see docs/SERVER_AUTH.md) so the header shape is reusable across the old and new stacks.
constexpr uint32_t HeaderMagic = 0x6B2E;
constexpr uint32_t HeaderSize = 24;
// Single-byte XOR mask applied only to the body. This is obfuscation for wire compatibility,
// explicitly not encryption.
constexpr uint8_t PayloadXorMask = 0xA7;

struct Connection {
    explicit Connection(SOCKET value) : socket(value) {}
    ~Connection() { closesocket(socket); }
    const SOCKET socket;
    std::string playerId, displayName;
    std::mutex writeMutex;
    // Monotonically increasing per-connection counter written into the header's sequence field
    // on every frame this side sends; the receiver does not require it to be gap-free. Only ever
    // touched while holding writeMutex, so the values observed on the wire are allocated in the
    // same order the underlying socket writes happen in (see Connection::send).
    uint32_t sendSequence{0};
    bool send(const protocol::Envelope& message);
    void stop() const { shutdown(socket, SD_BOTH); }
};
// Reads and validates exactly one framed Envelope. Returns false on any transport-level
// violation (short read, bad magic, nonzero entity, oversized payload, or a header/payload
// mismatch) — callers must treat false as "close the connection", not as a recoverable protocol
// error. The incoming sequence field is intentionally not validated: this contract does not
// require it to be gap-free or even nonzero (the legacy reference client always sends 0).
bool receive(SOCKET socket, protocol::Envelope& message);
protocol::SessionReply validate(const std::string& token, unsigned short platformPort);
}

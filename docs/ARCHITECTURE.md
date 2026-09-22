# MIMIC framework architecture

## Reference mapping

Inspected references: `C:/New/server` and `C:/NewClient/casino` (read only).

| Reference | MIMIC |
|---|---|
| FrontServer (C#/.NET Framework) | FrontServer (C#/.NET 8), discovery and health |
| PlatformServer (C#/.NET Framework) | PlatformServer (C#/.NET 8), development guest identity and sessions |
| TableServer (C++), Netlib | TableServer (C++20), Netlib TCP and WinHTTP adapters |
| ProtocolBuffer / MessageBuffer | One authoritative `ProtocolBuffer/proto/mimic.proto`; no MessageBuffer or MessagePack |
| Assets/_DEV/_Scripts/Managers | GaManager owns application flow and session data |
| Network / Net | NtManager, ProtoHttp, NetClient, Generated |
| Data / 0_Title / 1_Lobby / Holdem | SessionData, bootstrap, lobby and holdem views |
| Many game modules / vendor SDKs | Holdem only; no unrelated game or commercial asset copying |

## Flow

1. Unity GETs FrontServer `/portal` (binary Protobuf).
2. Unity POSTs PlatformServer `/auth/guest` with LoginRequest (Development only).
3. PlatformServer issues a cryptographically random one-hour bearer session token.
4. Unity connects to TableServer TCP and sends AuthenticateRequest.
5. TableServer POSTs the token to loopback PlatformServer `/sessions/validate`.
6. The authenticated connection can list, join, ready, act and leave.
7. TableServer owns all state and sends a separately redacted snapshot to each seated connection.

The client never chooses its player ID or initial chip balance. No token is stored in project configuration or logged. Idle connections expire after 90 seconds; the client sends a heartbeat every 20 seconds. Duplicate concurrent sessions are rejected. Request IDs are monotonically increasing per connection. Action requests include hand ID and table revision; stale actions cannot mutate state.

## Wire contract

TCP: 4-byte unsigned **big-endian** payload length followed by serialized Envelope. Maximum payload: 65,536 bytes. A zero length, truncated stream or invalid protobuf closes the connection. `protocol_version=1`. Requests use nonzero increasing request IDs; responses echo the ID; pushes use ID zero. Envelope uses a typed `oneof`; generated enums replace manually synchronized packet numbers. Unknown request types are rejected.

HTTP also uses `application/x-protobuf`. Health endpoints return JSON for operational inspection. The HTTP boundary does not reuse the TCP prefix. Protocol fields must never be renumbered; reserve removed tags.

## Holdem domain

One six-seat development table, 1,000 demo chips on joining, 10/20 blinds, server-side BCrypt shuffle, heads-up button rules, four betting rounds, showdown, 5-of-7 rank evaluation, unequal contribution pots and odd-chip allocation clockwise from the button. All seated players must ready up. Hole cards are visible only to their owner until a contested showdown. Disconnected non-all-in players fold; all-in hands remain eligible. Starting a new hand clears disconnected seats. No automatic resume is promised.

This is a **framework preview**, not a production poker engine. Full raises and short all-in calls are implemented. **Short all-in raises and reopening rights are deliberately rejected** until that rule is implemented and separately tested. Other missing features: turn timers, automatic table rotation, spectator mode, reconnect/resume, durable accounts/chips, buy-in/wallet ledger, rake, tournament rules and production login. A disconnect and rejoin can reset demo chips. Do not use these chips as a financial balance.

## Deployment boundaries

All services bind loopback by default. Guest login requires `ASPNETCORE_ENVIRONMENT=Development`. The development client allows HTTP only in the editor/development builds. Before remote hosting, replace local HTTP/TCP with authenticated TLS endpoints, real identity and persistence. Netlib currently uses Windows Winsock and WinHTTP; the domain is separated for a future IOCP/async or Linux adapter. A single world lock and bounded per-connection workers are sufficient for the preview, not a large-scale server.

## Dependencies

Unity 6000.3.22f1 matches the reference project. C# services target .NET 8. C++ uses MSVC 2022/C++20 and vcpkg Protobuf. Initial local verification uses the already installed vcpkg protobuf 3.21.12 compiler/runtime; the C# runtime is pinned to Google.Protobuf 3.28.2. These are development baselines, not a claim of current production security support. Upgrade compiler/runtime together and rerun cross-language tests before production.

Official references: [Protobuf C# generation](https://protobuf.dev/reference/csharp/csharp-generated/), [Protobuf C# basics](https://protobuf.dev/getting-started/csharptutorial/), [Unity plug-ins](https://docs.unity.com/en-us/engine/6000.3/manual/scripting/compilation-and-code-reload/plug-ins).

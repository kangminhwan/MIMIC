# MIMIC

Unity 6000.3.22f1 + native C++ TableServer, adapted for Texas Hold'em and Protocol Buffers.

[한국어 이전·실행 안내](docs/TABLESERVER_PROTOBUF_MIGRATION.md)

The default client now connects directly to the original TableServer protocol on port 22001. The original casino TCP transport and request queue are imported from `C:/NewClient/casino`; payload serialization uses Google.Protobuf. Blackjack, LowBaduki, Baccarat, Slot, Pinball and Roulette gameplay and dispatch have been removed from TableServer.

## Build

Requirements: Windows, Visual Studio 2022 C++ tools, .NET 8, Unity 6000.3.22f1, and the copied `Server/Include`, native libraries and `C:/vcpkg` dependencies. The repository's bundled protoc 3.21 matches the C++ protobuf headers.

```powershell
./Scripts/Build.ps1
./Scripts/Test-Smoke.ps1
./Scripts/Build-Unity.ps1 -Isolated
```

For just the server, use `./Scripts/Build-TableServer.ps1 -Configuration Debug` (or `Release`). MSBuild builds the matching Netlib configuration before TableServer. The server output is `Server/Bin/TableServer/TableServer.exe`.

## Configure and run

1. Configure the original database, Redis, lobby routing and client version in the native server environment. `Build-TableServer.ps1` initially copies `Server/TableServer/TableServer.xml` beside the executable and preserves any existing runtime copy.
2. Set `Client/Assets/Resources/Config/client.json`: `tableHost`, `tablePort`, `holdemChannel`, `holdemBetPolicy`, `appVersion` and `storeChannel`.
3. Run `./Scripts/Start-Local.ps1 -NoBuild` after the backend is ready. Check `artifacts/table.log` for startup status. Stop the launched process with `./Scripts/Stop-Local.ps1`.
4. Open `Client` in Unity and play the starting scene. Sign in with an existing original account. Quick start creates a Holdem room if no room is available. With at least two players, the player designated by the server as lead can start. Betting buttons follow the server's allowed actions.

The original account registration/identity verification service is required to create accounts. Native mode hides the unrelated demo registration screen. A native server is not authenticated by the retained C# demo account service.

## Protocol and verification

- Native wire schemas: `ProtocolBuffer/protoMessages/General.proto`, `PmNet.proto`, `Server.proto`, `Operation.proto`.
- `Scripts/Generate-Protocol.ps1` regenerates C++ and both C# consumers from those schemas.
- Requests contain raw protobuf messages. Replies contain `PmNet.PktBase`; the original 24-byte header and XOR framing remain compatible.
- `tests/TableServerClient` uses a loopback TCP peer to test fragmented frames, empty replies, Holdem state updates, server redirects, heartbeat, invalid packets and connection cleanup without a production database.
- The C# FrontServer/PlatformServer and `mimic.proto` are retained for the earlier demo and local screen models. They are separate from native TableServer authentication. Old demo instructions are archived in [DEMO_README_ARCHIVE.md](docs/DEMO_README_ARCHIVE.md).

See the migration guide for completed checks and runtime limits.

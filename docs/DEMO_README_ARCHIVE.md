> Archived demo documentation. The current default uses the native protobuf TableServer; commands below describe the retired demo stack.

# MIMIC

[한국어 실행 안내](docs/GETTING_STARTED.ko.md)

Unity + C++/C# Texas Hold'em framework. One game, one shared Protocol Buffers contract.

```text
Client/                       Unity 6000.3.22f1
  Assets/_DEV/_Scripts/       0_Title, 1_Lobby, Managers, Network, Data, Holdem, System
  Assets/_Scenes/             Bootstrap scene
Server/
  FrontServer/                C# .NET 8 discovery (127.0.0.1:5080)
  PlatformServer/             C# .NET 8 guest/session service (127.0.0.1:5081)
  TableServer/                C++20 authoritative Holdem (127.0.0.1:7777)
  Netlib/                     Bounded Protobuf TCP framing and session validation
ProtocolBuffer/proto/         Shared schema; C++ generated at build time
Scripts/                     Build, codegen, local run and integration test helpers
tests/                       C++ domain checks and real C#/C++ network smoke
```

## Requirements

- Windows, Visual Studio 2022 Desktop development with C++, Windows SDK, CMake >= 3.24.
- .NET 8 SDK; Unity 6000.3.22f1 with Windows Build Support.
- vcpkg with `protobuf:x64-windows` installed (`C:/vcpkg` by default).
- PowerShell. Run the commands below from the repository root.

## Build and test

```powershell
./Scripts/Build.ps1 -VcpkgRoot C:/vcpkg
./Scripts/Test-Smoke.ps1
./Scripts/Build-Unity.ps1
./Scripts/Test-UnitySmoke.ps1
```

`Build.ps1` regenerates C# from the shared proto, builds both C# services and the smoke client, builds C++ and runs domain tests, then syncs the Unity Protobuf runtime. Supply `-CMake` when CMake is elsewhere. The generated C# and Unity runtime DLL are committed so Unity can open the project without installing protoc first. C++ generated files stay in the ignored build folder.

## Run

```powershell
./Scripts/Start-Local.ps1 -NoBuild
```

Open `Client` with Unity Hub, open `Assets/_Scenes/Bootstrap.unity`, and press Play. Open `Client/Builds/Windows/MIMIC.exe` as the second player after building. Enter different display names, connect, join the table, and press Ready in both clients. Call/check through a hand, or fold/raise. The server deals and settles the hand, and each client sees only its own private cards. Choose Ready again for the next hand.

`MIMIC > Prepare project` recreates the bootstrap scene if missing; `MIMIC > Build Windows development client` builds from the editor. The HTTP discovery endpoint is configured in `Client/Assets/Resources/Config/client.json`. Local service logs and tracked process IDs are written only to ignored `artifacts/`.

```powershell
./Scripts/Stop-Local.ps1
```

## Scope

The structure follows the reference server/client roles while keeping only Holdem. The preview includes development guest login, sessions, a six-seat lobby/table, server-side shuffle and betting rounds, hand evaluation, pot settlement, per-player snapshots, heartbeat, framing validation, request correlation and tests. The IMGUI screen is a functional development harness; final art, uGUI/prefabs and mobile layout remain separate work.

**Not production-ready:** no real account/wallet persistence, automatic reconnect, turn timers or live deployment setup. Short all-in raises are explicitly rejected; short all-in calls and pot splitting are supported. All services default to localhost and demo chips only. See [architecture and limitations](docs/ARCHITECTURE.md) and [verification](docs/VERIFICATION.md).

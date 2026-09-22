# Verification

Local verification on 2026-09-22 (Windows):

- .NET 8: FrontServer, PlatformServer, generated protocol assembly and smoke harness build successfully.
- MSVC 2022/C++20: TableServer and Holdem tests build successfully.
- CTest: Holdem domain suite passes. Known hand ranks/kickers/wheel/tie, duplicate cards, turn checks, stale revisions, private cards, blinds, capacity, disconnect folds and all-in runouts are covered. Simulations exercise 150 fresh tables with up to six successive hands each and verify nonnegative stacks and chip conservation after every action. The number of assertions varies because shuffles are cryptographic.
- C#/C++ integration: discovery, guest login, unauthorized and duplicate sessions, lobby/join, private snapshots, full check/call hand through showdown, leave, concurrent requests, fragmented TCP packets, protocol-version rejection and oversized frame disconnect all pass.
- Unity 6000.3.22f1: editor compilation and Windows development player build pass with the included Protobuf/Unsafe runtime DLLs.
- Two **built Unity players**, running in batch mode, independently perform HTTP Protobuf login, C++ TCP authentication, ready, a full hand, private-card checks and settlement checks. Both emit `MIMIC_UNITY_SMOKE_PASS` and exit successfully.
- Local server startup/shutdown scripts verified to release ports 5080, 5081 and 7777.

Reproduce with `Scripts/Build.ps1`, `Scripts/Test-Smoke.ps1`, `Scripts/Build-Unity.ps1`, and `Scripts/Test-UnitySmoke.ps1`. Raw local logs are in ignored `artifacts/`. The GitHub workflow runs C++/.NET and network tests; it does **not** run Unity without a configured Unity license.

Not verified: visual UI inspection, Android/iOS/WebGL/IL2CPP builds, load/soak testing, external deployment, full official poker rules (short all-in raises are unsupported), reconnect/resume or production identity/persistence.

## Follow-up verification (2026-09-23)

The prior session could not invoke `cmake`/MSVC due to a tool-permission restriction (see
"Build verification" in `SERVER_AUTH.md`). This closes that gap and the `tests/Smoke` framing
uncertainty flagged in the same document:

- `cmake --build build --target Mimic.TableServer --config Release` — builds clean, 0 errors.
- `cmake --build build --target Mimic.Holdem.Tests --config Release` — builds clean, 0 errors.
- `ctest --test-dir build -C Release` — `holdem` suite passes (1/1).
- `Scripts/Test-Smoke.ps1` — full run against freshly started local services, 24/24 checks pass,
  including `Legacy header magic`, `Legacy fragmented framing and version rejection`, `Invalid
  legacy magic/nonce/command`. This confirms `tests/Smoke/Program.cs` already matches the 24-byte
  header framing end to end (source inspection also confirms it: `0x6B2E` magic at offset 0,
  little-endian, 24-byte header) — the "whether it has been updated ... status unknown" note in
  `SERVER_AUTH.md` is resolved.
- `dotnet run -c Release` in `tests/AccountTests` — all 109 checks pass (the one `fail:` line
  visible in the console output is Kestrel's own diagnostic log for the request the oversized-body
  test deliberately sends; the test assertion for that case passes).
- Local services were started and torn down by the scripts themselves; no process or port
  (5080/5081/7777) was left behind.

Also reviewed in this pass: the new client screens (`TitleScreen`, `LoginScreen`, `LobbyScreen`,
`TableCardView`, `HoldemScreen`, `HoldemSeatView`), `GaManager`/`NtManager`/`NetClient`/`ProtoHttp`,
and the copied `Network/Legacy/*` transport. The `Network/Legacy` files were diffed directly
against `C:/NewClient/casino`'s `Assets/_DEV/_Scripts/Network/Net/*`: `LinkTypes.cs` and
`IPayloadCipher.cs` are byte-identical; `PacketHeader.cs` is identical in content (the reference
copy carries a UTF-8 BOM, the MIMIC copy does not — encoding-only, not a behavioral difference);
`TcpLink.cs` likewise identical aside from the same BOM difference. This confirms `LEGACY_TCP.md`'s
"the original files ... are preserved" claim.

Two minor, non-blocking findings from that review, left as-is pending a decision from the
coordinator:
- `HoldemScreen.cs`'s raise handler resolves the acting player with `state.Players.First(...)`
  (throws if unmatched) where every other lookup in the same file uses `FirstOrDefault`
  defensively; a state update landing between button-enable and the click could throw unhandled.
- `GaManager.Navigate` lets a second concurrent caller's `await` return as soon as the first
  in-flight navigation finishes loading *some* scene, not necessarily the second caller's own
  `desiredScene` — the loop condition is a shared field, so the scene does eventually converge, but
  a caller that assumes "the active scene is `scene` right after this awaits" can be wrong for one
  frame if two navigations race.

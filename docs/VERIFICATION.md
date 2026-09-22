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

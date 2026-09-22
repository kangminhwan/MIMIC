# MIMIC 실행 안내

기존 서버의 C# FrontServer / C# PlatformServer / C++ TableServer 역할과 Unity의 Managers / Network / Data / Lobby / Holdem 구성을 유지한 홀덤 전용 기본 프레임워크입니다. 통신 데이터는 공통 `.proto`에서 생성한 Protocol Buffers를 사용합니다.

## 개발 환경

- Unity **6000.3.22f1**, Windows Build Support
- Visual Studio 2022 C++ 개발 도구, CMake, .NET 8 SDK
- `C:/vcpkg`의 `protobuf:x64-windows` (다른 경로는 `-VcpkgRoot`로 지정)

## 바로 실행

저장소 루트에서 PowerShell을 열고 다음 명령을 실행합니다.

```powershell
./Scripts/Build.ps1
./Scripts/Start-Local.ps1 -NoBuild
```

Unity Hub에서 `Client` 폴더를 추가합니다. `Assets/_Scenes/Bootstrap.unity`를 열고 Play를 누릅니다. 표시 이름을 입력해 Connect → Join → Ready 순서로 진행합니다. 2명 이상 입장하고 모두 Ready를 눌러야 시작합니다.

두 번째 클라이언트는 다음 명령으로 생성합니다.

```powershell
./Scripts/Build-Unity.ps1
```

생성된 `Client/Builds/Windows/MIMIC.exe`를 실행하면 에디터와 함께 테스트할 수 있습니다. 화면은 기본 기능 검증용이며, 최종 게임 UI·이미지·연출은 포함하지 않습니다.

## 검증과 종료

```powershell
./Scripts/Test-Smoke.ps1
./Scripts/Test-UnitySmoke.ps1
./Scripts/Stop-Local.ps1
```

스모크 테스트는 필요한 서버를 띄우고 종료하므로, 이미 직접 서버를 실행했다면 먼저 `Stop-Local.ps1`을 실행합니다. 이미 실행 중인 서버로 C#/C++ 테스트를 하려면 `Test-Smoke.ps1 -UseRunningServers`를 사용합니다.

서버 로그는 `artifacts/`, Unity 실행 파일은 `Client/Builds/Windows/`에 생성되며 Git에 올리지 않습니다. 기본 접속 주소는 `Client/Assets/Resources/Config/client.json`입니다.

## 현재 범위

개발용 로그인, 세션 인증, 로비, 6인 테이블, 준비, 블라인드, 폴드/체크/콜/정상 레이즈, 보드 진행, 패 평가, 팟 분배, 플레이어별 패 숨김, 하트비트가 구현되어 있습니다. 초기 칩은 테스트용 1,000개이며 입장 때 초기화됩니다.

다음은 추가 개발 항목입니다: 실제 계정·DB·재화 원장, 재접속 복구, 턴 제한 시간, 여러 테이블, 운영 배포, 최종 UI. **최소 레이즈보다 작은 올인 레이즈는 아직 거부**하며, 부족한 스택으로 올인 콜하는 흐름은 지원합니다. 현재 서버는 localhost 개발용으로 구성되어 있습니다.

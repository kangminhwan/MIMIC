# TableServer protobuf 이전 기록

작업일: 2026-09-23

## 적용한 구조

- 서버: `Server/TableServer`의 원본 C++ 구현을 사용한다. 기존 데모 `main.cpp`와 `Holdem/Table.cpp`를 복원하지 않았다.
- 메시지 본문: MessageBuffer/MessagePack 변환을 제거하고 `ParseFromArray`와 protobuf 직렬화를 사용한다.
- 클라이언트 요청은 개별 protobuf 메시지이고, 서버 응답은 `PmNet.PktBase`이다. 기존 24바이트 헤더와 XOR 프레이밍은 유지한다.
- 클라이언트 원본: `C:/NewClient/casino/Assets/_DEV/_Scripts/Network/Net`의 TCP 전송·요청 대기 코드를 가져와 protobuf 전용으로 수정했다. 홀덤 알림 처리는 원본 `Holdem/PM_HoldemBase.cs`와 서버의 실제 알림 순서를 참고했다.
- 현재 MIMIC 화면을 유지하면서 원본 서버용 로그인, 방 목록, 방 생성/입장/퇴장, 시작 권한 처리, 베팅, 카드 분배, 커뮤니티 카드, 쇼다운, 결과 확인을 연결했다.
- 좌석은 9인까지 표시한다. 임의 레이즈 금액 입력 대신 서버의 `PhaseTurnRS.wager_options`에 있는 액션을 표시한다.
- 다른 로비 서버로 이동하라는 응답을 받으면 계정 비밀번호를 다시 전송하지 않고 서버가 제공한 계정/플랫폼 식별자로 재인증한 뒤 원래 방 요청을 재시도한다. 자동 연결 복구는 기본적으로 꺼져 있다.

## 제거 범위

Blackjack, LowBaduki, Baccarat, Slot, Pinball, Roulette의 전용 소스/헤더, 핸들러, 프로젝트 참조, 룸 분기, 전용 데이터 로더 및 관련 슬롯 서버 연결·송신을 정리했다. ChannelData와 QuestData의 실행 데이터도 홀덤 범위로 줄였다.

공용 protobuf 스키마의 기존 필드 번호와 enum 값은 호환성을 위해 유지한다. DB 레코드의 공용 필드 및 홀덤이 직접 사용하는 `LowBadukiWait` 등의 기존 공용 설정 이름도 남아 있다. 이것이 해당 게임 실행 지원을 뜻하지는 않는다. `SeatSlot`은 홀덤 좌석을 뜻하므로 슬롯 게임으로 취급하지 않았다.

## 주요 파일

- `Client/Assets/_DEV/_Scripts/Network/TableServerClient.cs`: 원본 서버용 요청, 오류, 서버 이동, 세션 수명 처리.
- `Client/Assets/_DEV/_Scripts/Network/HoldemTableState.cs`: 서버 알림을 현재 UI 모델로 변환. 안티/블라인드와 카드 분배 전후 상태 알림 순서를 처리한다.
- `Client/Assets/_DEV/_Scripts/Network/Legacy/Core/NetClient.cs`: 원본 TCP 코어의 protobuf 코덱, 빈 응답, 하트비트, 중복 접속 및 오류 처리.
- `Client/Assets/_DEV/_Scripts/Network/NtManager.cs`: 현재 화면과 원본 서버 연결.
- `Client/Assets/_DEV/_Scripts/Holdem/NativeHoldemControls.cs`: 9인 좌석과 서버 지정 베팅 버튼.
- `Scripts/Generate-Protocol.ps1`: 저장소의 protoc 3.21로 C++ 및 두 C# 소비자의 코드를 함께 생성한다.
- `Scripts/Build-TableServer.ps1`: 같은 구성의 Netlib를 먼저 빌드하고 TableServer를 링크한다. DB 클라이언트 DLL, OpenSSL DLL 및 게임 데이터를 배치한다.

`mimic.proto`의 Envelope는 원본 TableServer에 전송하지 않는다. 현재 화면의 로컬 모델 및 남겨 둔 이전 데모 C# 서비스에서만 사용한다.

## 빌드와 실행

```powershell
./Scripts/Build.ps1
./Scripts/Build-TableServer.ps1 -Configuration Release
./Scripts/Test-Smoke.ps1
./Scripts/Build-Unity.ps1 -Isolated
```

서버 실행 파일: `Server/Bin/TableServer/TableServer.exe`.

`Build-TableServer.ps1`은 최초에 `Server/TableServer/TableServer.xml`을 실행 파일 옆으로 복사하며, 이미 존재하는 실행용 설정은 덮어쓰지 않는다. 실제 사용하는 설정은 `Server/Bin/TableServer/TableServer.xml`이다. 원본 DB 스키마, MySQL, Redis, 로비 서버 구성과 계정이 필요하다.

클라이언트 설정: `Client/Assets/Resources/Config/client.json`.

| 항목 | 초기값 | 의미 |
| --- | --- | --- |
| nativeTableServer | true | 원본 TableServer 프로토콜 사용 |
| tableHost / tablePort | 127.0.0.1 / 22001 | 접속할 원본 서버 |
| holdemChannel | Holdem_Chip_10K | 원본 ChannelData의 홀덤 채널 |
| holdemBetPolicy | 4 | HoldemStandard |
| appVersion | 1.0.0 | 서버의 버전 정책에 맞춰 설정 |
| storeChannel | 4 | PC |

환경을 구성한 뒤 `./Scripts/Start-Local.ps1 -NoBuild`로 서버를 실행한다. 이 스크립트의 실행 메시지는 프로세스를 시작했다는 뜻이며 DB 연결 및 준비 완료를 보장하지 않는다. `artifacts/table.log`에서 확인하고, `Stop-Local.ps1`로 해당 실행 기록의 프로세스를 종료한다.

Unity에서 기존 계정으로 로그인한다. 빈 로비에서 빠른 입장을 누르면 방을 생성한다. 2명 이상 입장하면 서버가 lead_idx로 지정한 플레이어가 시작할 수 있다. 결과 확인 버튼은 원본 서버의 `Packet_ResultViewDone`을 보낸다.

## 인증 및 검증 범위

- 원본 서버의 계정과 DB를 사용한다. 남아 있는 .NET PlatformServer의 데모 계정/토큰은 원본 TableServer 계정과 호환되지 않는다.
- 원본 회원가입·본인인증 서비스는 가져오지 않았다. Native 모드에서는 데모 회원가입 탭을 숨긴다. 기존 원본 계정으로 로그인하는 경로를 구현했다.
- 실제 원본 DB/Redis에 접속하거나 다중 클라이언트 실게임을 실행하지 않았다. 원본 환경의 접속 정보와 버전 정책이 맞는지 확인하는 실환경 검증은 별도로 필요하다.
- `tests/TableServerClient`는 외부 서비스 없이 루프백 TCP로 분할 수신, raw protobuf 요청, 빈 응답, 상태 갱신 순서, 9인 좌석, 안티/블라인드, 쇼다운, 리다이렉트/재인증, 하트비트, 중복 로그인 종료, 손상 패킷, 시간 초과 및 Dispose를 검증한다.
- 이전 `tests/Smoke`, `tests/HoldemTests.cpp`, Unity guest smoke는 이전 데모용이다. 현재 네이티브 프로토콜 검증은 `Test-Smoke.ps1`을 사용한다. `Test-UnitySmoke.ps1`은 Native 모드에서 이전 데모 테스트 실행을 명시적으로 거부한다.

## 백업과 빌드 관련 수정

- 원본 TableServer 백업: `C:/Users/WooJin/MIMIC-backups/TableServer-20260922-191351`.
- 클라이언트 화면 연결 전 백업: `C:/Users/WooJin/MIMIC-backups/Client-before-native`.
- Netlib 프로젝트 및 빌드 수정 전 파일: `C:/Users/WooJin/MIMIC-backups/Netlib-before-native-build.vcxproj`, `Netlib-build-fixes`.
- C++ 파일은 기존 한글 인코딩을 보존하는 바이트 단위 수정으로 처리했다.
- 원본 Netlib.lib는 Debug용이었다. 프로젝트 참조와 구성별 출력 경로를 추가해 Debug/Release 라이브러리가 섞이지 않도록 했다. 현재 컴파일러에서 드러난 문자열 const, goto 초기화 범위 오류를 수정했고 해당 WinInet 임시 버퍼를 해제하도록 정리했다.
- 원본 Redis/tacopie 라이브러리의 PDB 누락 경고는 링크를 막지 않으며, 해당 라이브러리 내부 디버깅 정보만 제한한다.

## 확인 결과

- TableServer Debug x64 + Netlib Debug: 전체 빌드 및 링크 통과 (`artifacts/native-server-debug-build.log`).
- TableServer Release x64 + Netlib Release: 전체 빌드 및 링크 통과 (`artifacts/native-server-release-build.log`).
- Unity 6000.3.22f1: 별도 검증 프로젝트에서 스크립트 컴파일 및 Windows Player 빌드 통과 (`artifacts/unity-native-player-build.log`).
- .NET 솔루션 Release 빌드와 기존 account tests 통과.
- 원본 프로토콜 클라이언트 테스트 7개 묶음 통과. 실제 서버 접속 결과와는 구분한다.

## Local deployment settings

Copy `Server/TableServer/LocalSettings.example.h` to `LocalSettings.local.h` and supply the existing deployment encryption keys and optional Slack webhook URLs before building. Missing encryption settings fail explicitly when used. Copy `TableServer.example.xml` to `TableServer.xml` and configure database connections before starting. Local settings, runtime output, caches and restored packages are excluded from Git. Required third-party libraries and the bundled protoc compiler are versioned.

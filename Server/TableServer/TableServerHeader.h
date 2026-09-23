#pragma once
#pragma warning(disable:4251)
#define NOMINMAX
#include "../Include/Netlib/Common/Netlib.h"

//#include "../Include/Common/ConstantTemplate.h"

#include "../Include/Common/CommonFlag.h"
//#include "../Include/Common/CommonDefine.h"
#include "../Include/Common/CommonStructure.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"
#include "../../ProtocolBuffer/cpp/server.pb.h"

//#include "flag.h"
//#include "Structure.h"

#pragma comment(lib, "Netlib.lib")

//#pragma comment(lib, "../Lib/ServerShared")

// vcpkg 로 설치한 폴더를 참조하도록 해 놨는데
// 환경에 따라 동일하게 적용되려면 어떻게 변경할지 고민이 필요해 보인다.
#ifdef _DEBUG
#pragma comment(lib, "C:/vcpkg/installed/x64-windows/debug/lib/libprotobufd.lib")
#pragma comment(lib, "C:/vcpkg/packages/openssl_x64-windows/debug/lib/libcrypto.lib")
#pragma comment(lib, "C:/vcpkg/packages/openssl_x64-windows/debug/lib/libssl.lib")
#else
#pragma comment(lib, "C:/vcpkg/installed/x64-windows/lib/libprotobuf.lib")
#pragma comment(lib, "C:/vcpkg/packages/openssl_x64-windows/lib/libcrypto.lib")
#pragma comment(lib, "C:/vcpkg/packages/openssl_x64-windows/lib/libssl.lib")
#endif
/*
#ifdef _DEBUG
#ifdef _M_X64
#pragma comment(lib, "./include/googleprotobuf/x64/googleprotobufD")
#else
#pragma comment(lib, "googleprotobuf/x86/googleprotobufD")
#endif
#else
#ifdef _M_X64
#pragma comment(lib, "googleprotobuf/x64/googleprotobuf")
#else
#pragma comment(lib, "googleprotobuf/x86/googleprotobuf")
#endif
#endif

*/

/*
[0x7FFFFFFE = 2147483646]
[0x7FFFFFFF <= int 표현범위 최대값]
*/
#define MAXCHANNEL 0x7FFFFFFE


// 개발용 define

//#define HOLDEM_LOWBADUKI_EMTPY_ROOM_CREATE // 바두기, 홀덤 빈방 미리 생성해두기 ( 개발서버에서만 활용 )


//#define INCREASE_CLIENT_TIME_OUT // 상점 구매창 처리시에 PING이 안오는 문제 처리를 위해 일시적으로 1분으로 늘린다.
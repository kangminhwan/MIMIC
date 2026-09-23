#pragma once
#include "../Common/Netlib.h"
#include "../Common/cSingleton.h"

BEGIN_NETLIB

class cMiniDump
{
public:
	static BOOL Begin();
	static BOOL End();

	static int ExceptionHandler(EXCEPTION_POINTERS* pException);
	static int StackExceptionHandler(EXCEPTION_POINTERS* pException);
	static std::wstringstream PrintStackTrace( CONTEXT* context );

	// Execption 생성기
	static void SoftwareException();

	static std::wstring stackTrace;

public:
	cMiniDump();
	virtual ~cMiniDump();
};

//typedef struct _MinidumpInfo
//{
//    DWORD threadId;    ///< crash가 발생한 threadId
//    EXCEPTION_POINTERS* pException;    ///< 예외 포인터
//	CMiniDump* pMiniDump;
//} MinidumpInfo, *PMinidumpInfo;

static unsigned __stdcall CreateMiniDump(LPVOID lParam)
{
	/*MinidumpInfo* pDumpinfo = (MinidumpInfo*)lParam;

	EXCEPTION_POINTERS* pException = pDumpinfo->pException;
	CMiniDump* pMiniDump = pDumpinfo->pMiniDump;*/

	EXCEPTION_POINTERS* info = reinterpret_cast<EXCEPTION_POINTERS*>(lParam);

	return cSingleton<cMiniDump>::GetInstance()->StackExceptionHandler(info);
}

END_NETLIB

/*
RaiseException();

void WINAPI RaiseException(

__in  DWORD dwExceptionCode,

__in  DWORD dwExceptionFlags,

__in  DWORD nNumberOfArguments,

__in  const ULONG_PTR *lpArguments

);

이 함수가 호출되면 SEH메커니즘이 작동되면서, 우리가 알고있는 예외처리가 전개된다.

소프트웨어 예외의 종류를 늘릴 때 사용. ( 하드웨어 예외는 우리가 추가할 수 없다 - 결정 되있음 ex)정수를 0으로 나누는 것 )




dwExceptionCode : 발생시킬 예외의 형태를 지정한다. ( 예외발생을 알리기 위한 용도로 사용 )

예외를 정의하는 기본적인 방식이 이미 정해져 있다.

가장 처음 부분을 0번째라고 하면

31,30번째 비트에는 예외의 심각도 수준을 나태는 것 ( 우리가 정함 느낌으로! )

( 00 - Success(성공) )

( 01 - Informational(예외의 알림) )

( 10 - Warning(예외에 대한 경고) )

( 11 - Error(강도 높은 오류 상황) )

29번째 비트에는 예외를 정의한 주체에 대한 정보를 담는다.

마이크로소프트가 정의한 예외는 0, 일반 개발자가 정의한 예외는 1 로 하자고 약속!

28번째 비트에는 시스템에 의해서 예약되어 있는 비트 - 0

16~27번째 비트에는 예외발생 환경 정보를 담게 된다. 다음 값 중에 하나를 지정

FACILITY_NULL		0

FACILITY_RPC		1

FACILITY_DISPATCH	2

FACILITY_STORAGE	3

FACILITY_ITF		4

FACILITY_WIN32		7

FACILITY_WINDOWS	8

FACILITY_SECURITY	9

FACILITY_CONTROL	10

FACILITY_CERT		11

FACILITY_INTERNET	12

FACILITY_MEDIASERVER	13

FACILITY_MSMQ		14

FACILITY_SETUPAPI	15

FACILITY_SCARD		16

FACILITY_COMPLUS	17

0~15번째 비트에는 예외의 종류를 구분하는 용도로 사용된다. 순수하게 정의할 내용은 여기에 해당

dwExceptionFlags : 0 - 특별한 설정을 하지 않는다는 뜻
EXCEPTION_NONCONTINUABLE - 예외가 발생한 시점에서부터의 실행을 막겠다는 뜻


세번째 네번째 인자는.. 그냥 NULL
*/


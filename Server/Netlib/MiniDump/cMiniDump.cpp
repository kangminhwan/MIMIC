#include "../../Include/Netlib/MiniDump/cMiniDump.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include <signal.h>
#include <windows.h>
#include <stdio.h>
#include <DbgHelp.h>
#include <string>
#include <iostream>
#include <sstream>

#define MINIDUMP_NOT_USE_FULL_DUMP

#pragma comment(lib, "Dbghelp.lib")

typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)( // Callback 함수의 원형
	HANDLE hProcess,
	DWORD dwPid,
	HANDLE hFile,
	MINIDUMP_TYPE DumpType,
	CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

LPTOP_LEVEL_EXCEPTION_FILTER PreviousExceptionFilter = NULL;
_invalid_parameter_handler oldHandler, newHandler;
terminate_handler terhandold, terhandnew;
unexpected_handler unexpecthandold, unexpecthandnew;

void raise_exception(const UINT excepitoncode)
{
	RaiseException(excepitoncode, 0, 0, NULL);
}

void handle_pure_virtual_function_call()
{
	raise_exception(EXCEPTION_ACCESS_VIOLATION);
}

void handle_interrupt_signal(int signal)
{
	raise_exception(EXCEPTION_ACCESS_VIOLATION);
}

LONG WINAPI UnHandledExceptionFilter(struct _EXCEPTION_POINTERS *exceptionInfo)
{
	HMODULE	DllHandle = NULL;

	// Windows 2000 이전에는 따로 DBGHELP를 배포해서 설정해 주어야 한다.
	DllHandle = LoadLibrary(_T("DBGHELP.DLL"));

	if(DllHandle)
	{
		MINIDUMPWRITEDUMP Dump = (MINIDUMPWRITEDUMP)GetProcAddress(DllHandle, "MiniDumpWriteDump");

		if(Dump)
		{
			TCHAR		DumpPath[MAX_PATH] = { 0 };
			SYSTEMTIME	SystemTime;

			GetLocalTime(&SystemTime);

			TCHAR* ptzAppName = NULL;
			NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
			if(pServerManager)
			{
				if(pServerManager->GetConfiguration())
					ptzAppName = pServerManager->GetConfiguration()->szAppName;
			}

			if(!ptzAppName)
				_sntprintf_s(	DumpPath, MAX_PATH, _T("[Revision %d]_%d-%d-%d %d_%d_%d.dmp"),
								SVN_REVISION,
								SystemTime.wYear,
								SystemTime.wMonth,
								SystemTime.wDay,
								SystemTime.wHour,
								SystemTime.wMinute,
								SystemTime.wSecond);
			else
				_sntprintf_s(	DumpPath, MAX_PATH, _T("[Revision %d]_%s %d-%d-%d %d_%d_%d.dmp"),
								SVN_REVISION,
								ptzAppName,
								SystemTime.wYear,
								SystemTime.wMonth,
								SystemTime.wDay,
								SystemTime.wHour,
								SystemTime.wMinute,
								SystemTime.wSecond);

			HANDLE FileHandle = CreateFile(
											DumpPath,
											GENERIC_WRITE,
											FILE_SHARE_WRITE,
											NULL, CREATE_ALWAYS,
											FILE_ATTRIBUTE_NORMAL,
											NULL);

			if(FileHandle != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION MiniDumpExceptionInfo;

#ifdef MINIDUMP_NOT_USE_FULL_DUMP
				// minidump, use MiniDumpNormal only
				// fulldump, use Flags below
				const DWORD Flags = MiniDumpNormal;
#else
				const DWORD Flags = MiniDumpWithFullMemory |
					MiniDumpWithFullMemoryInfo |
					MiniDumpWithHandleData |
					MiniDumpWithUnloadedModules |
					MiniDumpWithThreadInfo;
#endif

				MiniDumpExceptionInfo.ThreadId = GetCurrentThreadId();
				MiniDumpExceptionInfo.ExceptionPointers = exceptionInfo;
				MiniDumpExceptionInfo.ClientPointers = NULL;

				BOOL Success = Dump(
					GetCurrentProcess(),
					GetCurrentProcessId(),
					FileHandle,
					_MINIDUMP_TYPE(Flags),
					&MiniDumpExceptionInfo,
					NULL,
					NULL);

				if(Success)
				{
					CloseHandle(FileHandle);

					return EXCEPTION_EXECUTE_HANDLER;
				}
			}

			CloseHandle(FileHandle);
		}
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

void handle_invalid_parameter(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
	EXCEPTION_POINTERS ExceptionPointer;
	EXCEPTION_RECORD ExceptionRecord;
	_CONTEXT ContextRecord;

	ZeroMemory(&ContextRecord, sizeof(ContextRecord));
	RtlCaptureContext(&ContextRecord);

	ZeroMemory(&ExceptionRecord, sizeof(EXCEPTION_RECORD));
	ExceptionRecord.ExceptionCode = STATUS_INVALID_CRUNTIME_PARAMETER;

#ifdef _WIN64
	ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.Rip;
#else
	ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.Eip;
#endif

	ExceptionPointer.ExceptionRecord = &ExceptionRecord;
	ExceptionPointer.ContextRecord = &ContextRecord;

	UnHandledExceptionFilter(&ExceptionPointer);
}

void handle_set_terminate()
{
	EXCEPTION_POINTERS ExceptionPointer;
	EXCEPTION_RECORD ExceptionRecord;
	_CONTEXT ContextRecord;

	ZeroMemory(&ContextRecord, sizeof(ContextRecord));
	RtlCaptureContext(&ContextRecord);

	ZeroMemory(&ExceptionRecord, sizeof(EXCEPTION_RECORD));
	ExceptionRecord.ExceptionCode = STATUS_INVALID_CRUNTIME_PARAMETER;

#ifdef _WIN64
	ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.Rip;
#else
	ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.Eip;
#endif

	ExceptionPointer.ExceptionRecord = &ExceptionRecord;
	ExceptionPointer.ContextRecord = &ContextRecord;

	UnHandledExceptionFilter(&ExceptionPointer);
}

void handle_set_unexpected()
{
	EXCEPTION_POINTERS ExceptionPointer;
	EXCEPTION_RECORD ExceptionRecord;
	_CONTEXT ContextRecord;

	ZeroMemory(&ContextRecord, sizeof(ContextRecord));
	RtlCaptureContext(&ContextRecord);

	ZeroMemory(&ExceptionRecord, sizeof(EXCEPTION_RECORD));
	ExceptionRecord.ExceptionCode = STATUS_INVALID_CRUNTIME_PARAMETER;

#ifdef _WIN64
	ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.Rip;
#else
	ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.Eip;
#endif

	ExceptionPointer.ExceptionRecord = &ExceptionRecord;
	ExceptionPointer.ContextRecord = &ContextRecord;

	UnHandledExceptionFilter(&ExceptionPointer);
}

void handle_signal(int signal)
{
	EXCEPTION_POINTERS ExceptionPointer;
	EXCEPTION_RECORD ExceptionRecord;
	_CONTEXT ContextRecord;

	ZeroMemory(&ContextRecord, sizeof(ContextRecord));
	RtlCaptureContext(&ContextRecord);

	ZeroMemory(&ExceptionRecord, sizeof(EXCEPTION_RECORD));
	ExceptionRecord.ExceptionCode = STATUS_INVALID_CRUNTIME_PARAMETER;

#ifdef _WIN64
	ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.Rip;
#else
	ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.Eip;
#endif

	ExceptionPointer.ExceptionRecord = &ExceptionRecord;
	ExceptionPointer.ContextRecord = &ContextRecord;

	UnHandledExceptionFilter(&ExceptionPointer);
}

std::wstring NetLib::cMiniDump::stackTrace;

NetLib::cMiniDump::cMiniDump()
{
}


NetLib::cMiniDump::~cMiniDump()
{
}

BOOL NetLib::cMiniDump::Begin()
{
	SetErrorMode(SEM_FAILCRITICALERRORS);

	PreviousExceptionFilter = SetUnhandledExceptionFilter(UnHandledExceptionFilter);

	oldHandler = newHandler = NULL;
	newHandler = handle_invalid_parameter;
	oldHandler = _set_invalid_parameter_handler(newHandler);

	terhandold = terhandnew = NULL;
	terhandnew = handle_set_terminate;
	terhandold = set_terminate(handle_set_terminate);

	unexpecthandold = unexpecthandnew = NULL;
	unexpecthandnew = handle_set_unexpected;
	unexpecthandold = set_unexpected(unexpecthandnew);

	signal(SIGABRT, handle_signal);

	return true;
}

BOOL NetLib::cMiniDump::End()
{
	SetUnhandledExceptionFilter(PreviousExceptionFilter);

	return true;
}

int NetLib::cMiniDump::ExceptionHandler(EXCEPTION_POINTERS* pException)
{
	if(pException->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
	{
		/*   MinidumpInfo info;
		info.threadId = ::GetCurrentThreadId();
		info.pException = pException;*/

		// 덤프 찍기용 쓰레드 하나 생성해 주고
		//HANDLE hThread = (HANDLE)_beginthreadex(0, 1024*1024, CreateMiniDump, &info, 0, NULL);
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, CreateMiniDump, pException, 0, NULL);

		// 생성 쓰레드가 덤프 끝날 때까지 기다린다
		// 3초를 맥시멈으로 대기한다.
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
	}
	else
	{
		// 스택 오버플로우가 없는 일반적인 경우의 덤프 처리
		UnHandledExceptionFilter(pException);
		std::wstringstream ss = PrintStackTrace( pException->ContextRecord );
		stackTrace = ss.str();
	}

	//return EXCEPTION_EXECUTE_HANDLER;
	return EXCEPTION_CONTINUE_EXECUTION;
}

int NetLib::cMiniDump::StackExceptionHandler(EXCEPTION_POINTERS* pException)
{
	UnHandledExceptionFilter(pException);
	//return EXCEPTION_EXECUTE_HANDLER;
	return EXCEPTION_CONTINUE_EXECUTION;
}

void NetLib::cMiniDump::SoftwareException()
{
	DWORD DefinedException = 0x00;

	//Severity (31,30bit = 11 : Error)->강도 높은 오류상황.
	DefinedException |= 0x01 << 31;
	DefinedException |= 0x01 << 30;

	//MS or Customer (29bit = 1 : 일반 개발자 정의 예외) 
	DefinedException |= 0x01 << 29;

	//Reserved, Must be 0 (28bit = 0 : 시스템에 의해서 예약되어 있는 비트. 0으로 초기화해둠)
	DefinedException |= 0x00 << 28;

	//Facility code ( 27~16bit = 0 : FACILITY_NULL )
	DefinedException |= 0x00 << 16;

	//Exception code (15~0 bit = 0x08 : 예외의 종류를 구분짓는 용도 )
	DefinedException |= 0x08;
	DefinedException |= EXCEPTION_STACK_OVERFLOW;
	RaiseException(DefinedException, 0, NULL, NULL);
}

// 스택 정보를 출력하는 함수
std::wstringstream NetLib::cMiniDump::PrintStackTrace( CONTEXT* context )
{
	std::wstringstream ss;

	// 스택 정보를 포함한 기호를 가져오기 위한 핸들 생성
	HANDLE process = GetCurrentProcess();
	SymInitialize( process , NULL , TRUE );

	// 스택 프레임 정보를 가져올 수 있는 핸들 생성
	STACKFRAME64 stackFrame;
	ZeroMemory( &stackFrame , sizeof( stackFrame ) );

#ifdef _WIN64
	DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
	stackFrame.AddrPC.Offset = context->Rip;
	stackFrame.AddrFrame.Offset = context->Rbp;
	stackFrame.AddrStack.Offset = context->Rsp;
#else
	DWORD machineType = IMAGE_FILE_MACHINE_I386;
	stackFrame.AddrPC.Offset = context->Eip;
	stackFrame.AddrFrame.Offset = context->Ebp;
	stackFrame.AddrStack.Offset = context->Esp;
#endif

	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Mode = AddrModeFlat;

	// 스택 프레임 정보 출력
	while ( StackWalk64( machineType , process , GetCurrentThread() , &stackFrame , context , NULL , SymFunctionTableAccess64 , SymGetModuleBase64 , NULL ) )
	{
		DWORD64 address = stackFrame.AddrPC.Offset;
		ss << "Address: 0x" << std::hex << address << std::endl;

		// 기호 정보 가져오기
		DWORD64 displacement = 0;
		CHAR symbolBuffer[ sizeof( SYMBOL_INFO ) + MAX_SYM_NAME * sizeof( TCHAR ) ];
		PSYMBOL_INFO symbol = ( PSYMBOL_INFO ) symbolBuffer;
		symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
		symbol->MaxNameLen = MAX_SYM_NAME;

		if ( SymFromAddr( process , address , &displacement , symbol ) )
		{
			ss << "Symbol: " << symbol->Name << std::endl;
		}
		else
		{
			ss << "Symbol: Not found" << std::endl;
		}

		// 모듈 정보 가져오기
		IMAGEHLP_MODULE64 moduleInfo;
		ZeroMemory( &moduleInfo , sizeof( moduleInfo ) );
		moduleInfo.SizeOfStruct = sizeof( moduleInfo );

		if ( SymGetModuleInfo64( process , address , &moduleInfo ) )
		{
			ss << "Module: " << moduleInfo.ModuleName << std::endl;
		}
		else
		{
			ss << "Module: Not found" << std::endl;
		}

		ss << std::endl;
	}

	SymCleanup( process );
	return ss;
}

//https://sunhyeon.wordpress.com/2015/11/08/1899/ 참고 사이트

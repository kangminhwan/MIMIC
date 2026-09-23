// protobuftest.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include <iostream>
#include "TableServerHeader.h"
#include "cConfigReader.h"
#include "TableServerMain.h"

#include "tinyxml\tinyxml.h"

#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Common/AllocHook.h"
#include "../Include/Netlib/Manager/cThreadManager.h"

//#include <vld.h>
#ifdef _DEBUG
#include "cGuid.h"

#endif

namespace google
{
	namespace protobuf
	{
		class Message;
		class MessageLite;
	};
};



HANDLE hMutex = NULL;

BOOL ServerON = true;

TableServerMain* pServerMain = nullptr;
BOOL WINAPI ConsoleHandler( DWORD signal ) {
	if ( signal == CTRL_CLOSE_EVENT ) {
		ServerON = false;
		delete pServerMain;
		std::cout << "Console window is being closed!" << std::endl;
	}

	if ( NULL != hMutex )
	{
		ReleaseMutex( hMutex );
		CloseHandle( hMutex );
	}

	return TRUE;
}


int main()
{

#ifdef _DEBUG
	/*
	std::map<std::string , std::string> guidChecker;

	int loop = 0;
	std::string guid;
	while ( true )
	{
		// 테스트 코드 여기에
		guid = cGuild::GetGuid( 40 );
		guidChecker[ guid ] = guid;
		++loop;
	}
	*/
#endif
	if ( !SetConsoleCtrlHandler( ConsoleHandler , TRUE) ) {
		std::cerr << "Error: Could not set control handler." << std::endl;
		return 1;
	}
	//configReader::readXml();
	//static std::unique_ptr<TableServerMain> pServerMain = std::make_unique<TableServerMain>();
	pServerMain = new TableServerMain();
	if (nullptr == pServerMain)
		return 0;

	pServerMain->StartServer();
	
	if ( E_SERVER_STAGE::LIVE != NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerStage )
	{
		std::wstring mutex_name = L"TableServer" + std::to_wstring( NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->SID_FOR_MANAGE );

		hMutex = CreateMutex( NULL , TRUE , mutex_name.c_str() );

		if ( GetLastError() == ERROR_ALREADY_EXISTS )
		{
			std::wstring title = L"이미 로비" + std::to_wstring( NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->SID_FOR_MANAGE ) + L" 서버 실행 중입니다.";
			MessageBox( NULL , title.c_str() , L"알림" , MB_OK );

			if ( NULL != hMutex )
			{
				ReleaseMutex( hMutex );
				CloseHandle( hMutex );
			}

			return 0;
		}
	}

	//google::protobuf::MessageLite* pMessage = nullptr;

	//std::atexit( OnApplicationExit );
	while ( ServerON )
	{
		Sleep(1000);

		NetLib::cThreadManager* pThreadManager = NetLib::cSingleton<NetLib::cThreadManager>::ExistsInstance();
		if (pThreadManager)
		{
			// 모니터링할 쓰레드들만 추가합니다.
			pThreadManager->ReadThreadStatus(E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_COMMAND);
			pThreadManager->ReadThreadStatus(E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_WORKER);
			pThreadManager->ReadThreadStatus(E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_LOG);
			pThreadManager->ReadThreadStatus(E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_Web);
		}
	}
}

// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁:
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.

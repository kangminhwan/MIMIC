#include "../../Include/Netlib/Manager/cLogManager.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Common/cSingleton.h"

#include<iostream>

#if defined(UNICODE) || defined(_UNICODE)
#define _tcout std::wcout
#else
#define _tcout std::cout
#endif

using namespace std;

BOOL NetLib::cLogManager::outputConsole = FALSE;

NetLib::cLogManager::cLogManager() :
	m_hFile(INVALID_HANDLE_VALUE),
	m_bMoveLogFolder(FALSE),
	m_hUserLogFile(INVALID_HANDLE_VALUE)
{
	Open();
}


NetLib::cLogManager::~cLogManager()
{
	Close();
}

void NetLib::cLogManager::Close()
{

	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	if (m_hUserLogFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hUserLogFile);
		m_hUserLogFile = INVALID_HANDLE_VALUE;
	}
}

bool NetLib::cLogManager::Create(TCHAR* pFileName, TCHAR* pUserLogFileName)
{
	Close();

	// 현재 디렉토리가 log폴더 이면 그자리에 파일 생성
	// parent폴더일 경우에는 log폴더로 이동
	if(m_bMoveLogFolder == FALSE)
	{
		TCHAR path[MAX_PATH] = { 0, };
		//모듈의 경로를 받아옵니다.
		//첫번째 인수를 NULL로 해온다면 path변수에 전체 경로가 들어오게됩니다.
		//두번째는 자신의 실행경로를 받을 버퍼가 들어갑니다.(실행파일의 이름까지 받아옵니다.)
		//첫번째 변수가 NULL이 아니면 두번째 변수에 실행파일의 이름만 들어가게 됩니다.
		//첫번째 변수가 NULL일경우 두번째 변수에 전체 경로와 실행파일의 이름까지 들어가게 됩니다.
		//세번째는 실행 경로가 들어갈 버퍼의 크기를 넣으면됩니다.
		::GetModuleFileName(NULL, path, MAX_PATH);

		//위의 함수로 얻어오면 실행파일의 이름까지 얻어오게됩니다.
		//밑의 함수로 실행파일의 이름만 자르고 실행파일의 폴더 까지만 얻어오게됩니다.
		::PathRemoveFileSpec(path);
		//::GetCurrentDirectory(MAX_PATH, path);
		
		_tcscat_s(path, _T("\\log"));

		// log 폴더 이동 실패할경우, log폴더 생성한뒤 log폴더로 이동시킨다.
		if(!::SetCurrentDirectory(path))
		{

			// 생성 실패 폴더 만들어야 됨
			if(!CreateDirectory(path, NULL))
			{
				Assert(FALSE, _T("log folder create failed"));
				return false;
			}

			::SetCurrentDirectory(path);
		}

		m_bMoveLogFolder = TRUE;
	}

	if(_tcslen(pFileName))
		m_hFile = CreateFile(	pFileName,
								GENERIC_WRITE,
								FILE_SHARE_READ,
								NULL,
								CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								NULL);

	ServerManager* pServerManager = cSingleton<ServerManager>::ExistsInstance();
	if(pServerManager)
	{
		if(pServerManager->GetServerType() == E_SERVER_TYPE::LOBBY_SERVER ||
			pServerManager->GetServerType() == E_SERVER_TYPE::SLOT_SERVER)
		{
			if(_tcslen(pUserLogFileName))
				m_hUserLogFile = CreateFile(	pUserLogFileName,
												GENERIC_WRITE,
												FILE_SHARE_READ,
												NULL,
												CREATE_ALWAYS,
												FILE_ATTRIBUTE_NORMAL,
												NULL);
		}
	}

	if(m_hFile == INVALID_HANDLE_VALUE)
		return false;

	return true;
}

//TCHAR * cLogMgr::Open( TCHAR* pFileName )
TCHAR * NetLib::cLogManager::Open()
{
	memset(szLogFileName, 0, sizeof(szLogFileName));
	memset(szUserLogFileName, 0, sizeof(szUserLogFileName));

	GetLocalTime(&m_TSystemTime);

	// ServerConfiguration 파일의 정보 불러오기
	TServerConfiguration* pConfig = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance()->GetConfiguration();
	if(!pConfig)
	{
		::OutputDebugString(_T("cLogMgr::Open no server config"));
		return NULL;
	}

	wsprintf(	szLogFileName, _T("%s_%02d%02d_%02d%02d%02d.log")
				, pConfig->szAppName
				, m_TSystemTime.wMonth
				, m_TSystemTime.wDay
				, m_TSystemTime.wHour
				, m_TSystemTime.wMinute
				, m_TSystemTime.wSecond);

	wsprintf(	szUserLogFileName, _T("%s_%s_%02d%02d_%02d%02d%02d.log")
				, pConfig->szAppName
				, _T("Userlog")
				, m_TSystemTime.wMonth
				, m_TSystemTime.wDay
				, m_TSystemTime.wHour
				, m_TSystemTime.wMinute
				, m_TSystemTime.wSecond);

	if(!Create(szLogFileName, szUserLogFileName))
		return NULL;

	wPresentDay = m_TSystemTime.wDay;
	wPresentHour = m_TSystemTime.wHour;
	wPresentMin = m_TSystemTime.wMinute;

	// 유니코드 인코딩용 추가 시작
	// 이걸 먼저 써야지 text파일이 유니코드로 인코딩 된다.
	TCHAR unicode = 0xFEFF;
	DWORD dwWritten;
	WriteFile(m_hFile, &unicode, 2, &dwWritten, NULL);

	if(m_hUserLogFile)
		WriteFile(m_hUserLogFile, &unicode, 2, &dwWritten, NULL);
	// 유니코드 인코딩용 추가 끝


	return szLogFileName;
}

BOOL NetLib::cLogManager::Writes(TCHAR* pWriteBuffer, DWORD dwLen, BOOL bUserLog)
{
	DWORD dwWrite;

	if(bUserLog)
		return WriteFile(m_hUserLogFile, pWriteBuffer, dwLen, &dwWrite, NULL);
	else
		return WriteFile(m_hFile, pWriteBuffer, dwLen, &dwWrite, NULL);
}

void NetLib::cLogManager::WriteLog(TCHAR* format, BOOL bUserLog)
{
	if(format == nullptr)
		return;

	DWORD dwWrite;

	DWORD dwWriteByte = (DWORD)(_tcslen(format) * sizeof(TCHAR));

	if(bUserLog)
	{
		if(!WriteFile(m_hUserLogFile, format, dwWriteByte, &dwWrite, NULL))
		{
			// WriteFile 실패하였다..
			::OutputDebugString(_T("cLogMgr::WriteLog failed"));
			PrintLastError(__FILE__, __LINE__);
		}
	}
	else
	{
		if(!WriteFile(m_hFile, format, dwWriteByte, &dwWrite, NULL))
		{
			// WriteFile 실패하였다..
			::OutputDebugString(_T("cLogMgr::WriteLog failed"));
			PrintLastError(__FILE__, __LINE__);
		}
	}

	if (outputConsole)
	{
		_tprintf_s(format);
		//_tcout << format;
	}
}

// 날짜가 변경되었는지 체크한다. 변경되었으면 파일을 닫고 다시 쓴다.
BOOL NetLib::cLogManager::CheckDayChange()
{
	GetLocalTime(&m_TSystemTime);
	return m_TSystemTime.wDay != wPresentDay ? TRUE : FALSE;
}

BOOL NetLib::cLogManager::CheckHourChange()
{
	GetLocalTime(&m_TSystemTime);
	return m_TSystemTime.wHour != wPresentHour ? TRUE : FALSE;
}

BOOL NetLib::cLogManager::CheckMinChange()
{
	GetLocalTime(&m_TSystemTime);
	return m_TSystemTime.wMinute != wPresentMin ? TRUE : FALSE;
}
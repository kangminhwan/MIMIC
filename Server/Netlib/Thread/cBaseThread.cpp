#include "../../Include/Netlib/Thread/cBaseThread.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/MiniDump/cMiniDump.h"

NetLib::cBaseThread::cBaseThread() :
	m_byThreadCount(0)
{
	Init();
}


NetLib::cBaseThread::~cBaseThread()
{
	//상속 받은 클래스에서 콜해주고 있습니다.
	//Destroy();
}

void NetLib::cBaseThread::Init()
{
	memset(&m_uiThreadID, 0x00, sizeof(m_uiThreadID));
	memset(&m_hThread, 0x00, sizeof(m_hThread));
	memset(&m_uiThreadArray, 0x00, sizeof(m_uiThreadArray));
	memset(&m_dwTlsIndex, 0x00, sizeof(m_dwTlsIndex));

	SetClassName(_T("cBaseThread"));

	m_bTerminated = false;

	// Event초기화 
	m_hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hCloseEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// TLS 초기화
	/*if( (m_dwTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES )
	cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( RED, _T("%s TlsAlloc Failed"), m_tzClassName );

	assert( m_dwTlsIndex != TLS_OUT_OF_INDEXES );*/
}

void NetLib::cBaseThread::Destroy()
{
	Trace(_T("%s Threads Deleting\n"), m_tzClassName);

	// TLS 해제
	// TLS 해제
	for (int n = 0; n<MAX_THREAD_COUNT; ++n)
		if(m_dwTlsIndex[n])
			TlsFree(m_dwTlsIndex[n]);

	// if still alive, terminate thread
	if(!IsTerminated())
	{
		Terminate();
	}

	// close thread handle
	if(m_byThreadCount)
	{
		// 메세지 스레드 이벤트 닫기
		CloseHandle(m_hEvent);
		CloseHandle(m_hCloseEvent);

		for (int n = 0; n<m_byThreadCount; ++n)
		{
			if(m_hThread[n] != NULL)
				CloseHandle(m_hThread[n]);
		}
	}
}


//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

// Thread시작
bool NetLib::cBaseThread::StartThread(const int nThreadCnt, BOOL bCheckPriorityHigh)
{
	//Sleep(1000);
	if(nThreadCnt > MAX_THREAD_COUNT)
	{
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("StartThread Faliled, Reason is nThread %d Count is Too Big"), nThreadCnt);
		return false;
	}

	m_byThreadCount = nThreadCnt;

	for (int n = 0; n<nThreadCnt; ++n)
	{
		m_uiThreadArray[n] = n;

		m_hThread[n] = reinterpret_cast<HANDLE>(_beginthreadex(	NULL,
																0,
																&ThreadProc,
																this,
																0,
																&m_uiThreadID[n]));

		if(bCheckPriorityHigh)
		{
			BOOL bPriority = SetThreadPriority(m_hThread[n], THREAD_PRIORITY_HIGHEST);
			if(bPriority == FALSE)
			{
				printf("SetThreadPriority is failed\n");
				return false;
			}
		}

		if(m_hThread[n] == 0)
		{
			m_uiThreadID[n] = 0;
			return false;
		}

		//m_pThreadID_Map.insert( make_pair( m_uiThreadID[n], n) );
		//m_pThreadID_Map.SetAt( m_uiThreadID[n], n );
		//cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( _T("ThreadID %lu Created"), m_uiThreadID[n] );
	}

	return true;
}

unsigned __stdcall NetLib::cBaseThread::ThreadProc(void* lpHandle)
{
	unsigned uRetCode = 0;

	NetLib::cBaseThread* pThread = static_cast<NetLib::cBaseThread*>(lpHandle);

	NetLib::cMiniDump* m_pMiniDump = NetLib::cSingleton<NetLib::cMiniDump>::ExistsInstance();
	if(!m_pMiniDump)
	{
		printf("cBaseThread::ThreadProc CMiniDump GetInstance Failed\n");
		return 0;
	}

	// 이클라스의 몇번째 쓰레드 인지 찾는다.
	UINT uMyThreadArray = 0;
	for (int n = 0; n<MAX_THREAD_COUNT; ++n)
	{
		if(pThread->m_uiThreadID[n] == ::GetCurrentThreadId())
		{
			uMyThreadArray = pThread->m_uiThreadArray[n];
			break;
		}
	}

	__try
	{
		pThread->Process(uMyThreadArray);
	}
	__except (m_pMiniDump->ExceptionHandler(GetExceptionInformation()))
	{
		_tprintf(_T("%s cBaseThread::ThreadProc CMiniDump GetInstance Failed\n"), pThread->m_tzClassName);
	}

	_endthreadex(uRetCode);
	return uRetCode;
}

void NetLib::cBaseThread::SendThreadCloseEvent()
{
	for (int n = 0; n<m_byThreadCount; ++n)
		::SetEvent(m_hCloseEvent);
}

// 스레드 강제 종료 
void NetLib::cBaseThread::ForceThreadKill()
{
	DWORD dwExitCode;

	for (int n = 0; n<m_byThreadCount; ++n)
	{
		GetExitCodeThread(m_hThread[n], &dwExitCode);
		if(dwExitCode == STILL_ACTIVE)
			::TerminateThread(m_hThread[n], 0);
	}
}

bool NetLib::cBaseThread::WaitForAllThreadTermination()
{
	bool bWait = true;
	int nStoppedThread = 0;
	WORD wTryCount = TRY_THREAD_TERMINATE; // 이숫자 만큼 기다려보고, 실패하면 강제로 쓰레드 죽인다.

	while (bWait)
	{
		Sleep(1);
		for (int n = 0; n<m_byThreadCount; ++n)
		{
			if(m_uiThreadID[n] == 0) ++nStoppedThread;

			if(nStoppedThread == m_byThreadCount)
			{
				bWait = false;
			}
		}

		--wTryCount;
		if(wTryCount == 0)
		{
			return false;
		}
	}

	return true;
}